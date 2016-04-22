#include <getopt.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tair_client_api_impl.hpp"
#include <sys/syscall.h>
#include <sys/types.h>
#include <vector>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <getopt.h>
#include <pthread.h>
#include <tbsys.h>
#include <easy_io.h>
#include "define.hpp"
#include "log.hpp"
#include "query_info_packet.hpp"

#define peek_sleep(stop_, now, delay)           \
  {                                             \
    while (time(NULL) < now + delay)            \
    {                                           \
      if (stop_) break;                         \
      usleep(10000);                            \
    }                                           \
  }
using namespace std;

namespace tair
{

  string itostr(uint64_t n)
  {
    char s[21];
    snprintf(s, sizeof(s), "%lu", n);
    return s;
  }

  typedef size_t (*response_callback_pt)(void *buffer, size_t size, size_t nmemb, void *userp);

  class HttpClient;
  class HttpRequest
  {
  public:
    HttpRequest(CURL *curl);
    ~HttpRequest();
  public:
    void set_url(const char *url);
    void add_header(const char *header);
    void set_post_fields(const char *fields);
    void set_post_fields(const char *fields, size_t len);
    void set_method_put();
  private:
    friend class HttpClient;
    struct curl_slist *headers_;
    CURL *curl_;
  };

  class HttpClient
  {
  public:
    HttpClient();
    ~HttpClient();
  public:
    HttpRequest* new_request();
    CURLcode do_request(HttpRequest *req, response_callback_pt cb = NULL, void *userdata = NULL);
  private:
    CURL *curl_;
  };

//~ HttpRequest
  HttpRequest::HttpRequest(CURL *curl)
  {
    headers_ = NULL;
    this->curl_ = curl;
  }

  HttpRequest::~HttpRequest()
  {
    if (headers_ != NULL)
    {
      curl_slist_free_all(headers_);
    }
  }
  void HttpRequest::set_url(const char *url)
  {
    curl_easy_setopt(curl_, CURLOPT_URL, url);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
  }

  void HttpRequest::add_header(const char *header)
  {
    headers_ = curl_slist_append(headers_, header);
  }

  void HttpRequest::set_post_fields(const char *fields)
  {
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, fields);
  }

  void HttpRequest::set_post_fields(const char *fields, size_t len)
  {
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, fields);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, len);
  }

  void HttpRequest::set_method_put()
  {
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
  }

//~ HttpClient
  HttpClient::HttpClient()
  {
    curl_ = curl_easy_init();
  }

  HttpClient::~HttpClient()
  {
    curl_easy_cleanup(curl_);
  }

  HttpRequest* HttpClient::new_request()
  {
    return new HttpRequest(this->curl_);
  }

  CURLcode HttpClient::do_request(HttpRequest *req, response_callback_pt cb, void *userdata)
  {
    if (req->headers_ != NULL)
    {
      curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, req->headers_);
    }
    if (cb != NULL)
    {
      curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, cb);
    }
    if (userdata != NULL)
    {
      curl_easy_setopt(curl_, CURLOPT_WRITEDATA, userdata);
    }
    return curl_easy_perform(curl_);
  }

  class GroupInfo
  {
  public:
    GroupInfo(const string &cluster = "")
      :cluster_name(cluster)
    {
      master = slave = 0;
      ping_count = 8;
      interval = 60;
      rotate_size = 1073741824; //1GB
      monitor_enable = false;
    }
  public:
    uint64_t master;
    uint64_t slave;
    string group_name;
    string cluster_name;

    int ping_count;
    int timeout;
    int interval;
    int64_t rotate_size;

    string log_area;
    string log_server;
    string log_server_total;

    bool monitor_enable;
    string monitor_url;
  };

  // store tair stat
  class Stat
  {
    enum
    {
      server,
      area
    };

  public:
    int load_data(tair_client_impl* client, const GroupInfo* group)
    {
      if (NULL == client || NULL == group)
        return TAIR_RETURN_FAILED;
      int ret;
      ret = client->query_from_dataserver(0, tair_stat_);
      if (TAIR_RETURN_FAILED == ret)
        return ret;
      load_server_ping_stat(client, group);
      load_area_quota(client, group);
      return ret;
    }

    const vector<string>& get_stat_desc() const
    {
      return tair_stat_.get_stat_desc();
    }

    const map<string, vector<int64_t> >& get_server_area_stat() const
    {
      return tair_stat_.get_server_area_stat();
    }

    const map<string, vector<int64_t> >& get_area_stat() const
    {
      return tair_stat_.get_area_stat();
    }

    const map<string, vector<int64_t> >& get_server_stat()
    {
      return tair_stat_.get_server_stat();
    }

    const map<string, int64_t>& get_server_ping_stat()
    {
      return server_ping_stat_;
    }

    const map<string, int64_t>& get_area_quota()
    {
      return area_quota_;
    }

  private:
    void load_server_ping_stat(tair_client_impl* client, const GroupInfo* group)
    {
      map<string, int64_t> tmp_server_ping_stat;
      set<uint64_t> ds_set;
      client->get_servers(ds_set);

      int ping_count = group->ping_count;
      for (set<uint64_t>::const_iterator it = ds_set.begin(); it != ds_set.end(); ++it)
      {
        string ip_str = tbsys::CNetUtil::addrToString(*it);
        // retrieve 'ping_time'
        int64_t total = 0;
        int64_t max_ping = -1;
        int64_t *ping_times = new int64_t[ping_count];

        for (int i = 0; i < ping_count; ++i)
        {
          ping_times[i] = client->ping(*it);
          total += ping_times[i];
          if (ping_times[i] > max_ping)
            max_ping = ping_times[i];
        }

        //ping_time = total / group->ping_count;
        int64_t ping_time = (total - max_ping) / (ping_count - 1);

        // log each ping as ping time too large
        if (ping_time > 10000)
        {
          log_error("ping to %s in %s abnormal", ip_str.c_str(), group->cluster_name.c_str());
          for (int i = 0; i < ping_count; ++i)
            log_error("the %dnd time : ping : %f", i, ping_times[i] / 1000.);
        }
        delete[] ping_times;

        tmp_server_ping_stat[ip_str] = ping_time;
      }
      server_ping_stat_.swap(tmp_server_ping_stat);
    }

    void load_area_quota(tair_client_impl* client, const GroupInfo* group)
    {
      map<string, string> raw_area_quota_stat;
      map<string, int64_t> tmp_area_quota;
      client->query_from_configserver(tair::request_query_info::Q_AREA_CAPACITY, group->group_name.c_str(), raw_area_quota_stat, 0);
      for (map<string, string>::const_iterator it = raw_area_quota_stat.begin(); it != raw_area_quota_stat.end(); ++it)
      {
        string::size_type pos_start = it->first.find_first_of('(');
        string::size_type pos_end = it->first.find_last_of(')');
        if (string::npos == pos_start || string::npos == pos_end)
          continue;
        string area = it->first.substr(pos_start + 1, pos_end - (pos_start + 1));
        tmp_area_quota[area] = (int64_t)atol(it->second.c_str());
      }
      area_quota_.swap(tmp_area_quota);
    }

  private:
    map<string, int64_t> server_ping_stat_;
    map<string, int64_t> area_quota_;
    tair::tair_statistics tair_stat_;
  };

  // send tair stat json string to monitor
  class Monitor
  {
  public:
    Monitor()
    {
      context_buffer_.reserve(4096);
    }

    void set_url(const string& url)
    {
      url_ = url;
    }

    bool push(Stat& stat_data)
    {
      // from a json
      context_buffer_.clear();
      stat_to_json(stat_data);

      HttpRequest *req = client_.new_request();
      req->add_header("Content-Type:text/plain");
      req->set_url(url_.c_str());
      req->set_post_fields(context_buffer_.c_str());
      CURLcode err = client_.do_request(req);
      if (err != CURLE_OK)
      {
        log_error("Push Moniter failed:%s", curl_easy_strerror(err));
        return false;
      }
      delete req;
      return true;
    }

  private:
    void stat_to_json(Stat& stat_data)
    {
      const map<string, vector<int64_t> >& server_area_stat = stat_data.get_server_area_stat();
      const map<string, int64_t>& server_ping_stat = stat_data.get_server_ping_stat();
      const map<string, int64_t>& area_quota = stat_data.get_area_quota();

      /*
        for example:
        {
        ds_area_stat:
        [
        {
        ds_area: "10.69.69.101:16004:65535",
        stat: [1, 2 ,3, ]
        },
        {
        ds_area: "10.69.69.102:16004:65535",
        stat: [4, 5 ,6, ]
        },
        ],
        server_ping:
        [
        {
        ds: "10.69.69.101:16004",
        ping: 12345
        },
        {
        ds: "10.69.69.102:16004",
        ping: 12346
        }
        ],
        area_quota:
        [
        {
        area: "65534",
        quota: 112220000
        },
        {
        area: "65534",
        quota: 112220000
        },
        ]
        }
      */
      context_buffer_.append("{");

      //add ds_area_stat
      context_buffer_.append("ds_area_stat:[");
      for (map<string, vector<int64_t> >::const_iterator it = server_area_stat.begin(); it != server_area_stat.end();
           ++it)
      {
        context_buffer_.append("{ds_area:\"" + it->first + "\",");
        context_buffer_.append("stat: [");
        for (vector<int64_t>::const_iterator item_iter = it->second.begin(); item_iter != it->second.end();
             ++item_iter)
        {
          char tmp[24];
          snprintf(tmp, sizeof(tmp), "%ld,", *item_iter);
          context_buffer_.append(tmp);
        }
        context_buffer_.append("]},");
      }
      context_buffer_.append("],"); //correspond to context_buffer_.append("ds_area_stat: [")

      /*
        append server_ping
      */
      context_buffer_.append("server_ping:[");
      for (map<string, int64_t>::const_iterator it = server_ping_stat.begin(); it != server_ping_stat.end(); ++it)
      {
        context_buffer_.append("{ds:\"" + it->first + "\",");
        char ping_time[24];
        snprintf(ping_time, sizeof(ping_time), "%ld", it->second);
        context_buffer_.append("ping:" + string(ping_time) + "},");
      }
      context_buffer_.append("],"); // correspond to context_buffer_.append("ping_data:[")

      /*
        append area_quota
       */
      context_buffer_.append("area_quota:[");
      for (map<string, int64_t>::const_iterator it = area_quota.begin(); it != area_quota.end(); ++it)
      {
        context_buffer_.append("{area:\"" + it->first + "\",");
        char quota[24];
        snprintf(quota, sizeof(quota), "%ld", it->second);
        context_buffer_.append("quota:" + string(quota) + "},");
      }
      context_buffer_.append("]"); // correspond to context_buffer_.append("area_quota:[")

      context_buffer_.append("}");
    }

  private:
    HttpClient client_;
    string url_;
    string context_buffer_;
  };

  class Lens
  {
  public:
    typedef map<pthread_t, GroupInfo*> pid_group_map_t;
    typedef map<pthread_t, tair::tair_client_impl*> pid_client_map_t;

  public:
    Lens();
    ~Lens();

  public:
    void start();
    void stop()
    {
      purge_groups();
    }
    bool running()
    {
      return !stop_;
    }
    bool init(const char *filename);
    void reload();
    void update_clients();

  private:
    static void* hook(void *args);
    void write_server_stat(const char* time_str, Stat& stat_data, int fd_server);
    void write_server_total_stat(const GroupInfo* group, const char* time_str, Stat& stat_data, int fd_server_total);
    void write_area_stat(const char* time_str, Stat& stat_data, int fd_area);
    void rotate_log(int64_t rotate_size, const char* old_file, const char* time_str, int& fd);
    void worker(int idx);
    bool load_groups();
    void purge_groups();
    bool checkout_dir(const char *path);

  private:
    volatile bool stop_;
    int index;
    pthread_mutex_t mutex;
    vector<GroupInfo*> groups;
    pid_client_map_t pid_client_map;
    string config_file;
  };

  Lens::Lens()
  {
    stop_ = false;
    index = 0;
    pthread_mutex_init(&mutex, NULL);
  }

  Lens::~Lens()
  {
    pthread_mutex_destroy(&mutex);
  }

  bool Lens::init(const char *filename)
  {
    config_file = filename;
    return load_groups();
  }

  bool Lens::load_groups()
  {
    //~ local CConfig object
    tbsys::CConfig config;
    if (config.load(config_file.c_str()) != EXIT_SUCCESS)
    {
      log_error("config file %s format error", config_file.c_str());
      return false;
    }

    const char *log_dir = config.getString("public", "log_dir", "logs");
    const char *log_area = config.getString("public", "log_area", "log_area.log");
    const char *log_server = config.getString("public", "log_server", "log_server.log");
    const char *log_server_total = config.getString("public", "log_server_total", "log_server_total.log");

    bool ret = false;
    vector<string> sections;
    config.getSectionName(sections);
    //~ allocate a 'group' for each 'section' except 'public'
    for (vector<string>::iterator it = sections.begin(); it != sections.end(); ++it)
    {
      if (*it == "public")
        continue;

      log_info("loading group for cluster '%s'...", it->c_str());
      GroupInfo *group = new GroupInfo(*it);
      do
      {
        const char *s = NULL;
        //~ master & slave
        s = config.getString(it->c_str(), "master");
        if (s == NULL)
        {
          log_error("cluster %s: 'master' cannot be empty, thus ignored.", it->c_str());
          delete group;
          break;
        }
        char *t = strdup(s);
        vector<char*> ip_port_str;
        tbsys::CStringUtil::split(t, ":", ip_port_str);
        group->master = tbsys::CNetUtil::strToAddr(ip_port_str[0], atoi(ip_port_str[1]));
        s = config.getString(it->c_str(), "slave");
        free(t);
        t = strdup(s);
        ip_port_str.clear();
        tbsys::CStringUtil::split(t, ":", ip_port_str);
        group->slave = s == NULL ? 0 : tbsys::CNetUtil::strToAddr(ip_port_str[0], atoi(ip_port_str[1]));
        free(t);
        //~ group name
        s = config.getString(it->c_str(), "group_name");
        if (s == NULL)
        {
          log_error("cluster %s: 'group_name' cannot be empty, thus ignored.", it->c_str());
          delete group;
          break;
        }
        group->group_name = s;
        log_debug("group_name: %s.", s);

        //~ log by area
        if (config.getInt(it->c_str(), "need_log_area", 0) != 0)
        {
          char fn[1024];
          snprintf(fn, sizeof(fn), "%s/%s/%s", log_dir, it->c_str(), log_area);
          //~ make sure path of file exists
          if (!checkout_dir(fn))
          {
            delete group;
            break;
          }
          group->log_area = fn;
        }

        //~ log by server
        if (config.getInt(it->c_str(), "need_log_server", 0) != 0)
        {
          char fn[1024];
          snprintf(fn, sizeof(fn), "%s/%s/%s", log_dir, it->c_str(), log_server);
          //~ make sure path of file exists
          if (!checkout_dir(fn))
          {
            delete group;
            break;
          }
          group->log_server = fn;
        }

        //~ log by cluster
        if (config.getInt(it->c_str(), "need_log_server_total", 0) != 0)
        {
          char fn[1024];
          snprintf(fn, sizeof(fn), "%s/%s/%s", log_dir, it->c_str(), log_server_total);
          //~ make sure path of file exists
          if (!checkout_dir(fn))
          {
            delete group;
            break;
          }
          group->log_server_total = fn;
        }

        group->ping_count = config.getInt(it->c_str(), "ping_count", 10);
        group->interval = config.getInt(it->c_str(), "interval", 60);
        group->timeout = config.getInt(it->c_str(), "timeout", 200);
        group->rotate_size = atol(config.getString("public", "log_rotate_size", "1073741824"));
        log_info("cluster '%s' parameters, ping_count: %d, interval: %ds, timeout: %dms, rotate_size: %ld.",
                 group->cluster_name.c_str(), group->ping_count, group->interval, group->timeout, group->rotate_size);

        group->monitor_enable = config.getInt(it->c_str(), "monitor_enable", 0) != 0 ? true : false;
        if (group->monitor_enable)
        {
          const char* url = config.getString(it->c_str(), "monitor_url");
          if (NULL == url)
          {
            log_error("cluster %s: 'url_monitor' cannot be empty, if monitor_enable is true.", it->c_str());
            delete group;
            break;
          }
          group->monitor_url = url;
        }

        groups.push_back(group);
        ret = true;
        log_info("cluster '%s' loaded.", it->c_str());
      } while (false);
    } //~ for
    return ret;
  }

//~ close 'clients', join 'threads', free 'groups'
  void Lens::purge_groups()
  {
    stop_ = true;
    pthread_mutex_lock(&mutex);
    for (pid_client_map_t::iterator it = pid_client_map.begin(); it != pid_client_map.end(); ++it)
    {
      log_warn("joining thread %lu...", it->first);
      pthread_join(it->first, NULL);
      if (it->second != NULL)
      {
        it->second->close();
        delete it->second;
      }
    }
    pthread_mutex_unlock(&mutex);
    //! bug may occur here.
    pid_client_map.clear();
    for (vector<GroupInfo*>::iterator it = groups.begin(); it != groups.end(); ++it)
      delete *it;
    groups.clear();
  }

//~ make sure that the directory part of 'path' exists
  bool Lens::checkout_dir(const char *path)
  {
    char *p, dirpath[256];
    snprintf(dirpath, sizeof(dirpath), "%s", path);
    p = strrchr(dirpath, '/');
    if (p != NULL)
      *p = '\0';
    if (p != NULL && !tbsys::CFileUtil::mkdirs(dirpath))
    {
      log_error("'mkdirs' %s failed: %m.", dirpath);
      return false;
    }
    return true;
  }

//~ 'purge_groups', then 'load_groups', then 'start'
  void Lens::reload()
  {
    //if (groups.empty() || groups.size() != pid_client_map.size()) {
    //return ;
    //}
    purge_groups();

    log_info("reloading config file '%s'", config_file.c_str());
    if (!load_groups())
    {
      log_error("reload groups failed.");
    }
    log_info("restarting threads");
    index = 0;
    stop_ = false;
    start();
  }

//~ update 'server table' of each client, if necessary
  void Lens::update_clients()
  {
    const char *pkey = "lens_counter";
    tair::data_entry key(pkey, strlen(pkey), false);
    int value;
    for (pid_client_map_t::iterator it = pid_client_map.begin(); it != pid_client_map.end(); ++it)
    {
      it->second->add_count(0, key, 1, &value);
      it->second->remove(0, key);
    }
    log_warn("update clients completed.");
    return;
  }

//~ launch worker threads
  void Lens::start()
  {
    for (size_t i = 0; i < groups.size(); ++i)
    {
      pthread_t pid;
      usleep(100000);
      pthread_create(&pid, NULL, Lens::hook, this);
    }
  }

//~ callback 'worker'
  void* Lens::hook(void *args)
  {
    Lens *lens = (Lens*) args;
    lens->worker(__sync_fetch_and_add(&lens->index, 1));
    return NULL;
  }

  // write stat to server_log
  void Lens::write_server_stat(const char* time_str, Stat& stat_data, int fd_server)
  {
    if (fd_server < -1)
      return;

    const vector<string>& stat_desc = stat_data.get_stat_desc();
    const map<string, vector<int64_t> >& server_stat = stat_data.get_server_stat();

    if (server_stat.empty())
      return;

    const map<string, int64_t>& server_ping_stat = stat_data.get_server_ping_stat();

    // print server info
    for (map<string, vector<int64_t> >::const_iterator it = server_stat.begin(); it != server_stat.end(); ++it)
    {
      //~ form a record
      char record[1024];
      int32_t record_len = 0;
      record_len = snprintf(record, sizeof(record), "time:%s,dataserver:%s,", time_str, it->first.c_str());
      for (size_t i = 0; i < stat_desc.size(); ++i)
      {
        record_len += snprintf(record + record_len, sizeof(record) - record_len, "%s:%ld,", stat_desc[i].c_str(),
                               it->second[i]);
      }

      map<string, int64_t>::const_iterator ping_iter = server_ping_stat.find(it->first);
      int64_t ping_time = ping_iter != server_ping_stat.end() ? ping_iter->second : 0;
      record_len += snprintf(record + record_len, sizeof(record) - record_len, "ping:%f,stat:%d\n", ping_time / 1000.0,
                             1);
      write(fd_server, record, record_len);
    }
  }

  // write server_total_log
  void Lens::write_server_total_stat(const GroupInfo* group, const char* time_str, Stat& stat_data, int fd_server_total)
  {
    // print server total info, cluster info
    if (fd_server_total < 0)
      return;

    const vector<string>& stat_desc = stat_data.get_stat_desc();
    const map<string, vector<int64_t> >& server_stat = stat_data.get_server_stat();

    if (server_stat.empty())
      return;
    const map<string, int64_t>& server_ping_stat = stat_data.get_server_ping_stat();

    // get total server stat
    vector<int64_t> total_stat(server_stat.begin()->second.size(), 0);
    for (map<string, vector<int64_t> >::const_iterator it = server_stat.begin(); it != server_stat.end(); ++it)
    {
      for (size_t i = 0; i < it->second.size(); ++i)
        total_stat[i] += it->second[i];
    }

    // get total ping
    int64_t total_ping_time = 0;
    for (map<string, int64_t>::const_iterator it = server_ping_stat.begin(); it != server_ping_stat.end(); ++it)
      total_ping_time += it->second;

    //~ form total record
    char record[1024];
    int32_t record_len = snprintf(record, sizeof(record), "time:%s,cluster:%s,", time_str, group->cluster_name.c_str());
    for (size_t i = 1; i < stat_desc.size(); ++i)
    {
      record_len += snprintf(record + record_len, sizeof(record) - record_len, "%s:%ld,", stat_desc[i].c_str(),
                             total_stat[i]);
    }
    record_len += snprintf(record + record_len, sizeof(record) - record_len, "ping:%f\n",
                           total_ping_time / (1000.0 * server_stat.size()));
    //~ write to file
    write(fd_server_total, record, record_len);
  }

  // write stat to area_log
  void Lens::write_area_stat(const char* time_str, Stat& stat_data, int fd_area)
  {
    if (fd_area < 0)
      return;

    const vector<string>& stat_desc = stat_data.get_stat_desc();
    const map<string, vector<int64_t> >& area_stat = stat_data.get_area_stat();

    //print
    for (map<string, vector<int64_t> >::const_iterator it = area_stat.begin(); it != area_stat.end(); ++it)
    {
      //~ form a record
      char record[1024];
      int32_t record_len = snprintf(record, sizeof(record), "time:%s,area:%s,", time_str, it->first.c_str());
      for (size_t i = 0; i < stat_desc.size(); ++i)
      {
        record_len += snprintf(record + record_len, sizeof(record) - record_len, "%s:%ld,", stat_desc[i].c_str(),
                               it->second[i]);
      }
      record[record_len - 1] = '\n';
      write(fd_area, record, record_len);
    }
  }

  void Lens::rotate_log(int64_t rotate_size, const char* old_file, const char* time_str, int& fd)
  {
    struct stat sb;
    fstat(fd, &sb);
    if ((int64_t)sb.st_size > rotate_size)
    {
      string new_file(old_file);
      new_file.append(".");
      new_file.append(time_str);
      if (rename(old_file, new_file.c_str()) != 0)
      {
        log_error("rename %s to %s failed: %m.", old_file, new_file.c_str());
        return;
      }
      close(fd);
      fd = open(old_file, O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
      if (-1 == fd)
        log_error("open %s failed: %m.", old_file);
    }
  }

  //~ worker thread to retrieve stat info
  void Lens::worker(int idx)
  {
    GroupInfo *group = groups[idx];
    log_info("thread %ld, group %s", syscall(SYS_gettid), group->group_name.c_str());
    tair::tair_client_impl* client = new tair::tair_client_impl();
    client->set_timeout(group->timeout);
    string master_str = tbsys::CNetUtil::addrToString(group->master);
    string slave_str = tbsys::CNetUtil::addrToString(group->slave);
    log_info("thread %d starts, cluster: %s, master: %s, slave: %s.", idx, group->cluster_name.c_str(),
             master_str.c_str(), slave_str.c_str());
    if (!client->startup(master_str.c_str(), slave_str.c_str(), group->group_name.c_str()))
    {
      log_error("client 'startup' failed. cluster: %s, thread %d.", group->cluster_name.c_str(), idx);
      delete client;
      return;
    }
    pthread_mutex_lock(&mutex);
    pid_client_map[pthread_self()] = client;
    pthread_mutex_unlock(&mutex);
    //~ open log files.
    int fd_area = -1, fd_server = -1, fd_server_total = -1;
    if (!group->log_area.empty())
    {
      fd_area = open(group->log_area.c_str(), O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
      if (fd_area == -1)
        log_error("open %s failed: %m.", group->log_area.c_str());
    }
    if (!group->log_server.empty())
    {
      fd_server = open(group->log_server.c_str(), O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
      if (fd_server == -1)
        log_error("open %s failed: %m.", group->log_server.c_str());
    }
    if (!group->log_server_total.empty())
    {
      fd_server_total = open(group->log_server_total.c_str(), O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0644);
      if (fd_server_total == -1)
        log_error("open %s failed: %m.", group->log_server_total.c_str());
    }

    Monitor *mon = NULL;
    if (group->monitor_enable)
    {
      mon = new Monitor();
      mon->set_url(group->monitor_url);
    }

    while (!stop_)
    {
      //~ get timestamp
      time_t t;
      time(&t);
      struct tm tm;
      localtime_r(&t, &tm);
      char time_str[32];
      snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,
               tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

      Stat stat_data;
      // fetch tair stat
      if (stat_data.load_data(client, group) != TAIR_RETURN_FAILED)
      {
        // write stat to server_log
        if (fd_server >= 0)
        {
          write_server_stat(time_str, stat_data, fd_server);
          rotate_log(group->rotate_size, group->log_server.c_str(), time_str, fd_server);
        }
        // write to server_total_log
        if (fd_server_total >= 0)
        {
          write_server_total_stat(group, time_str, stat_data, fd_server_total);
          rotate_log(group->rotate_size, group->log_server_total.c_str(), time_str, fd_server_total);
        }
        // write stat to area_log
        if (fd_area >= 0)
        {
          write_area_stat(time_str, stat_data, fd_area);
          rotate_log(group->rotate_size, group->log_area.c_str(), time_str, fd_area);
        }

        if (mon != NULL)
        {
          // send stat to Monitor
          mon->push(stat_data);
        }
      }
      else
        log_error("tair %s, group %s: get stat failed", group->cluster_name.c_str(), group->group_name.c_str());
      //~ stop_, or sleep for a while
      peek_sleep(stop_, t, group->interval);
    } // end of while(!stop_)

    if (mon != NULL)
      delete mon;

    if (fd_area > 0)
      close(fd_area);
    if (fd_server > 0)
      close(fd_server);
    if (fd_server_total > 0)
      close(fd_server_total);
  }

} // endof namespace tair

void* monitor(void *args);
void sig_handler(int sig);
char *parse_cmd_line(int argc, char * const argv[]);
void print_usage(char *prog_name);

tair::Lens *lens;

int main(int argc, char **argv)
{
  const char *configfile = parse_cmd_line(argc, argv);
  if (configfile == NULL)
  {
    print_usage(argv[0]);
    return 1;
  }
  if (TBSYS_CONFIG.load(configfile))
  {
    fprintf(stderr, "load ConfigFile %s failed.\n", configfile);
    return 1;
  }

  const char *pidFile = TBSYS_CONFIG.getString("public", "pid_file", "lens.pid");
  const char *logFile = TBSYS_CONFIG.getString("public", "log_file", "lens.log");

  char *p, dirpath[256];
  snprintf(dirpath, sizeof(dirpath), "%s", pidFile);
  p = strrchr(dirpath, '/');
  if (p != NULL)
    *p = '\0';
  if (p != NULL && !tbsys::CFileUtil::mkdirs(dirpath))
  {
    fprintf(stderr, "mkdirs %s failed.\n", dirpath);
    return 1;
  }
  snprintf(dirpath, sizeof(dirpath), "%s", logFile);
  p = strrchr(dirpath, '/');
  if (p != NULL)
    *p = '\0';
  if (p != NULL && !tbsys::CFileUtil::mkdirs(dirpath))
  {
    fprintf(stderr, "mkdirs %s failed.\n", dirpath);
    return 1;
  }

  int pid;
  if ((pid = tbsys::CProcess::existPid(pidFile)))
  {
    fprintf(stderr, "process already running, pid:%d\n", pid);
    return 1;
  }

  const char *logLevel = TBSYS_CONFIG.getString("public", "log_level", "info");
  TBSYS_LOGGER.setLogLevel(logLevel);

  if (tbsys::CProcess::startDaemon(pidFile, logFile) == 0)
  {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(40, sig_handler);
    signal(41, sig_handler);
    signal(42, sig_handler);
    signal(43, sig_handler);

    lens = new tair::Lens;
    if (lens->init(configfile) == false)
    {
      log_error("'lens' init failed.");
      delete lens;
      lens = NULL;
      return 1;
    }
    lens->start();
    pthread_t pid;
    pthread_create(&pid, NULL, monitor, const_cast<char*>(configfile));
    pthread_join(pid, NULL);
  }
  return 0;
}

//~ thread monitoring the 'configfile'
void* monitor(void *args)
{
  const char *configfile = (const char *) args;
  struct stat st;
  stat(configfile, &st);
  time_t last_mtime = st.st_mtime;
  while (lens->running())
  {
    sleep(1);
    stat(configfile, &st);
    if (last_mtime != st.st_mtime)
    {
      log_warn("%s is modified, reloading.", configfile);
      lens->reload();
      last_mtime = st.st_mtime;
    }
  }
  return NULL;
}
//~ parse the command line
char *parse_cmd_line(int argc, char * const argv[])
{
  int opt;
  const char *optstring = "hVf:";
  struct option longopts[] =
    {
      { "config_file", 1, NULL, 'f' },
      { "help", 0, NULL, 'h' },
      { "version", 0, NULL, 'V' },
      { 0, 0, 0, 0 } };

  char *configFile = NULL;
  while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1)
  {
    switch (opt)
    {
      case 'f':
        configFile = optarg;
        break;
      case 'V':
        fprintf(stderr, "BUILD_TIME: %s %s\n\n", __DATE__, __TIME__);
        exit(1);
      case 'h':
        print_usage(argv[0]);
        exit(1);
    }
  }
  return configFile;
}

//~ signal handler
void sig_handler(int sig)
{
  switch (sig)
  {
    case SIGTERM:
    case SIGINT:
      lens->stop();
      break;
    case 40:
      TBSYS_LOGGER.checkFile();
      break;
    case 41:
    case 42:
      if (sig == 41)
      {
        TBSYS_LOGGER._level++;
      }
      else
      {
        TBSYS_LOGGER._level--;
      }
      log_error("log level changed to %d.", TBSYS_LOGGER._level);
      break;
    case 43:
      lens->update_clients();
  }
}

//~ print the help information
void print_usage(char *prog_name)
{
  fprintf(stderr, "%s -f config_file\n"
          "    -f, --config_file  config file\n"
          "    -h, --help         show this message\n"
          "    -V, --version      show version\n\n", prog_name);
}
