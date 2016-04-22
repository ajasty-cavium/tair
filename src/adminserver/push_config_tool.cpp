#include <iostream> 
#include <fstream> 
#include <sstream>
#include <curl/curl.h>

using namespace std;

int main(int argc, char *argv[])
{
  stringstream ssurlbase;
  ssurlbase << "http://" << argv[1] << "/set/?tid=" << argv[2] << "&cid=" << argv[3];
  string urlbase = ssurlbase.str();
  std::fstream f(argv[4]);
  while (f.eof() == false && f.fail() == false)
  {
    std::string key;
    std::string val;
    f >> key >> val;
    if (f.fail())
      break;
    stringstream ss;
    ss << urlbase << "&key=" << key << "&val=" << val;
    CURL *curl = curl_easy_init();
    std::string url = ss.str();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
    CURLcode err = curl_easy_perform(curl);
    if (err != CURLE_OK)
    {
      cout << "FAILED:" << url << endl;
    }
    else
      cout << "SUCCED:" << url << endl;
  }
  f.close();

  return 0;
}

