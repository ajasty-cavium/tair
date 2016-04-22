#include "com_taobao_tair_jmdb_JMdbNative.h"

#include <string.h>
#include "tbsys.h"
#include "libmdb_c.hpp" 

const char *mdb_type = "mdb";
const char *mdb_path = "/dev/shm/";

const char *out_of_memory_clazz = "java/lang/OutOfMemoryError";
const char *peer_is_null_mesg   = "mdb instance peer is null";

static 
data_entry_t GetDataEntry(JNIEnv *env, jbyteArray array)
{
  static jboolean is_copy = JNI_FALSE;  
  data_entry_t entry = {0, NULL};
  entry.data = reinterpret_cast<char *>(env->GetByteArrayElements(array, &is_copy));
  if (entry.data == NULL) 
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), 
        "get jbyteArray elements failed"); 
    return entry;
  }
  entry.size = env->GetArrayLength(array);
  return entry; 
}

static
void DropDataEntry(JNIEnv *env, jbyteArray array, data_entry_t *entry)
{
  if (entry->data == NULL)
    return;
  env->ReleaseByteArrayElements(array, reinterpret_cast<jbyte *>(entry->data), JNI_ABORT);
}

JNIEXPORT jlong JNICALL Java_com_taobao_tair_jmdb_JMdbNative_initialize
    (JNIEnv *, jclass, jlong msize)
{
  if (msize < 500)
    return 0;
  mdb_param_t param;
  bzero(&param, sizeof(param));
  param.size         = (int64_t)(msize) * 1024 * 1024; // MB 
  param.use_check_thread = true; 
  param.lock_pshared = false;
  param.inst_shift   = 0;
  param.mdb_type     = mdb_type;
  param.mdb_path     = mdb_path;
  param.slab_base_size = 8;       // 128Byte -> 1MB
  param.page_size    = 1024 * 1024; // 1MB
  param.factor       = 1.5;
  param.hash_shift   = 23;
  param.chkexprd_time_low  = 4;
  param.chkexprd_time_high = 5;
  param.chkslab_time_low   = 5;
  param.chkslab_time_high  = 6;
  // disable log
  TBSYS_LOGGER._level = -1;
 
  mdb_t inst = mdb_init(&param);
  return reinterpret_cast<jlong>(inst); 
}

JNIEXPORT jlong JNICALL Java_com_taobao_tair_jmdb_JMdbNative_datasize
  (JNIEnv *env, jclass, jlong peer, jshort area)
{
  if (peer == 0)
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), peer_is_null_mesg);
    return -1;
  }
  mdb_t inst = reinterpret_cast<mdb_t>(peer);
  mdb_stat_t stat;
  mdb_get_stat(inst, area, &stat);
  return (long)(stat.data_size);
}

JNIEXPORT jlong JNICALL Java_com_taobao_tair_jmdb_JMdbNative_usage
  (JNIEnv *env, jclass, jlong peer, jshort area)
{
  if (peer == 0)
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), peer_is_null_mesg);
    return -1;
  }
  mdb_t inst = reinterpret_cast<mdb_t>(peer);
  mdb_stat_t stat;
  mdb_get_stat(inst, area, &stat);
  return (long)(stat.space_usage);
}


JNIEXPORT void JNICALL Java_com_taobao_tair_jmdb_JMdbNative_destroy
  (JNIEnv *env, jclass, jlong peer)
{
  if (peer == 0)
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), peer_is_null_mesg);
    return;
  }
  mdb_t inst = reinterpret_cast<mdb_t>(peer);
  mdb_destroy(inst);
}

JNIEXPORT void JNICALL Java_com_taobao_tair_jmdb_JMdbNative_put
  (JNIEnv *env, jclass clazz, jlong peer, jshort area, jbyteArray key, jbyteArray val)
{
  if (peer == 0)
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), peer_is_null_mesg);
    return;
  }
  if (area < 0) return;
  mdb_t inst = reinterpret_cast<mdb_t>(peer);
  data_entry_t key_entry = GetDataEntry(env, key);
  data_entry_t val_entry = GetDataEntry(env, val);
  if (key_entry.data != NULL && val_entry.data != NULL)
  {
    mdb_put(inst, area, &key_entry, &val_entry, 0, 0, 0);
  }
  DropDataEntry(env, key, &key_entry);
  DropDataEntry(env, val, &val_entry);
}

JNIEXPORT jbyteArray JNICALL Java_com_taobao_tair_jmdb_JMdbNative_get
  (JNIEnv *env, jclass clazz, jlong peer, jshort area, jbyteArray key)
{
  if (peer == 0)
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), peer_is_null_mesg);
    return NULL;
  }
  if (area < 0) return NULL;

  mdb_t inst = reinterpret_cast<mdb_t>(peer);
  data_entry_t val_entry;
  bzero(&val_entry, sizeof(data_entry_t));
  jbyteArray jvalue = NULL;
  int rc = -1;
  data_entry_t key_entry = GetDataEntry(env, key);
  if (key_entry.data != NULL)  // check whether GetEntry is valid
  {
    rc = mdb_get(inst, area, &key_entry, &val_entry, NULL, NULL); 
  }
  DropDataEntry(env, key, &key_entry);

  if (rc == 0 && val_entry.data != NULL) // TAIR_RETURN_SUCCESS
  {
    jvalue = env->NewByteArray(val_entry.size);
    if (jvalue == NULL)
    {
      env->ThrowNew(
          env->FindClass(out_of_memory_clazz), 
          "jvm can't alloc byte[]");
    }
    else
    {
      env->SetByteArrayRegion(jvalue, 0, val_entry.size, (jbyte *)(val_entry.data));
    }
  }
  if (val_entry.data != NULL)
    free(val_entry.data);
  return jvalue;
}

JNIEXPORT void JNICALL Java_com_taobao_tair_jmdb_JMdbNative_capacity
  (JNIEnv *env, jclass clazz, jlong peer, jshort area, jlong cap)
{
  if (peer == 0)
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), peer_is_null_mesg);
    return;
  }
  if (area < 0) return;
  if (cap  < 0 || cap > 1024 * 24) return;

  mdb_t inst = reinterpret_cast<mdb_t>(peer);
  uint64_t quota = (uint64_t)(cap) * 1024 * 1024; // MB 
  mdb_set_quota(inst, area, quota); 
}

JNIEXPORT void JNICALL Java_com_taobao_tair_jmdb_JMdbNative_delete
  (JNIEnv *env, jclass clazz, jlong peer, jshort area, jbyteArray key)
{
  if (peer == 0)
  {
    env->ThrowNew(
        env->FindClass(out_of_memory_clazz), peer_is_null_mesg);
    return;
  }
  if (area < 0) return;
  
  mdb_t inst = reinterpret_cast<mdb_t>(peer);
  data_entry_t key_entry = GetDataEntry(env, key);
  if (key_entry.data != NULL)
  {
    mdb_del(inst, area, &key_entry, false); // not care version
  }
  DropDataEntry(env, key, &key_entry);
}


