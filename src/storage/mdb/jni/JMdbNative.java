package com.taobao.tair.jmdb;

public class JMdbNative {

  //
  public native long initialize(long msize);

  public native void destroy(long peer);

  public native void put(long peer, short area, byte[] key, byte[] val);

  public native byte[] get(long peer, short area, byte[] key);

  public native void delete(long peer, short area, byte[] key);

  public native void capacity(long peer, short area, long cap);

  public native long datasize(long peer, short area);

  public native long usage(long peer, short area);

  public void loadLibrary() {
    System.loadLibrary("mdb_j");
  }
}

