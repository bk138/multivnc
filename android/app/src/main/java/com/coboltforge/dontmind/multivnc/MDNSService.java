package com.coboltforge.dontmind.multivnc;

/*
 * @author Christian Beier
 * mDNS Service Discovery Service.
 * Copyright © 2011-2023 Christian Beier <dontmind@freeshell.org>
 */

import java.util.Hashtable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Semaphore;

import android.annotation.SuppressLint;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import androidx.annotation.NonNull;

import com.coboltforge.dontmind.multivnc.db.ConnectionBean;

public class MDNSService extends Service {

	private final String TAG = "MDNSService";
	private final IBinder mBinder = new LocalBinder();

	private NsdManager nsdManager;
	// we use a separate worker thread to do the resolve OPs one after another, see
	// https://stackoverflow.com/a/68424163/361413
	private final MDNSWorkerThread workerThread = new MDNSWorkerThread();

	private OnEventListener callback;

	public interface OnEventListener {
		void onMDNSstartupCompleted(boolean wasSuccessful);
		void onMDNSnotify(final String conn_name, final ConnectionBean conn, final Hashtable<String,ConnectionBean> connectionTable);
	}

	/**
	 * Class for clients to access. Because we know this service always runs in
	 * the same process as its clients, we don't need to deal with IPC.
	 */
	public class LocalBinder extends Binder {
		public MDNSService getService() {
			return MDNSService.this;
		}
	}




	@Override
	public IBinder onBind(Intent intent) {
		// handleIntent(intent);
		return mBinder;
	}



	@Override
	public void onCreate() {
		//code to execute when the service is first created
		Log.d(TAG, "mDNS service onCreate()!");

		nsdManager = (NsdManager) getSystemService(Context.NSD_SERVICE);
		workerThread.start();
	}


	@Override
	public void onDestroy() {
		//code to execute when the service is shutting down
		Log.d(TAG, "mDNS service onDestroy()!");

		// this will end the worker thread
		workerThread.interrupt();
	}


	@Override
	public int onStartCommand(Intent intent, int flags, int startid) {
		//code to execute when the service is starting up
		Log.d(TAG, "mDNS service onStartCommand()!");

		if(intent == null)
			Log.d(TAG, "Restart!");

		return START_STICKY;
	}

	// NB: this is called from the worker thread!!
	public void registerCallback(OnEventListener c) {
		callback = c;
	}

	// force a callback call for every conn in connections_discovered
	public void dump() {
		try {
			Message.obtain(workerThread.handler, MDNSWorkerThread.MESSAGE_DUMP).sendToTarget();
		}
		catch(NullPointerException e) {
			//unused
		}
	}

	public void restart() {
		Message.obtain(workerThread.handler, MDNSWorkerThread.MESSAGE_STOP).sendToTarget();
		Message.obtain(workerThread.handler, MDNSWorkerThread.MESSAGE_START).sendToTarget();
	}


	private class MDNSWorkerThread extends Thread
	{
		private android.net.wifi.WifiManager.MulticastLock multicastLock;
		private static final String mdnstype = "_rfb._tcp";
		private NsdManager.DiscoveryListener discoveryListener;
		private NsdManager.ResolveListener resolveListener;
		// work around the fact that only one resolve OP can be active at one time
		// https://stackoverflow.com/a/68424163/361413
		private final Semaphore resolveSemaphore = new Semaphore(1);
		// accessed from both worker thread and nsdManager thread
		private final ConcurrentHashMap<String,ConnectionBean> connections_discovered = new ConcurrentHashMap<>();
		private Handler handler;

		final static int MESSAGE_START = 0;
		final static int MESSAGE_STOP = 1;
		final static int MESSAGE_DUMP = 2;
		final static int MESSAGE_RESOLVE = 3;


		// this just runs a message loop and acts according to messages
		@SuppressLint("HandlerLeak") // no handler leak as looper runs on worker thread
		public void run() {

			resolveListener = new NsdManager.ResolveListener() {
				@Override
				public void onResolveFailed(NsdServiceInfo serviceInfo, int errorCode) {
					resolveSemaphore.release();

					Log.e(TAG, "Resolve for " + serviceInfo + " failed with code " + errorCode);
				}

				@Override
				public void onServiceResolved(NsdServiceInfo serviceInfo) {
					resolveSemaphore.release();

					ConnectionBean c = new ConnectionBean();
					c.id = 0; // new!
					c.nickname = serviceInfo.getServiceName();
					c.address = serviceInfo.getHost().toString().replace('/', ' ').trim();
					c.port = serviceInfo.getPort();
					c.useLocalCursor = true; // always enable

					connections_discovered.put(serviceInfo.getServiceName(), c);

					Log.d(TAG, "Resolve succeeded for server :" + serviceInfo.getServiceName()
							+ ", now " + connections_discovered.size());

					mDNSnotify(serviceInfo.getServiceName(), c);
				}
			};

			Looper.prepare();

			handler = new Handler() {

				public void handleMessage(@NonNull Message msg) {
					// if interrupted, bail out at once
					if(isInterrupted())
					{
						Log.d(TAG, "INTERRUPTED, bailing out!");
						mDNSstop();
						getLooper().quit();
						return;
					}
					// otherwise, process our message queue

					switch (msg.what) {
					case MDNSWorkerThread.MESSAGE_START:
						mDNSstart();
						break;
					case MDNSWorkerThread.MESSAGE_STOP:
						mDNSstop();
						break;
					case MDNSWorkerThread.MESSAGE_DUMP:
						for(ConnectionBean c : connections_discovered.values()) {
							mDNSnotify(c.nickname, c);
						}
						break;
					case MESSAGE_RESOLVE:
						try {
							resolveSemaphore.acquire();
							nsdManager.resolveService((NsdServiceInfo) msg.obj, resolveListener);
						} catch (InterruptedException e) {
							Log.d(TAG, "INTERRUPTED, bailing out!");
							mDNSstop();
							getLooper().quit();
							return;
						}
						break;
					}
				}

			};

			// start on our own
			mDNSstart();

			Looper.loop();
		}


		private void mDNSstart()
		{
			Log.d(TAG, "starting MDNS");

			if(discoveryListener != null) {
				Log.d(TAG, "MDNS already running, doing nothing");
				try{
					callback.onMDNSstartupCompleted(false);
				}
				catch(NullPointerException e) {
					Log.d(TAG, "callback is NULL, not notified about startup");
				}
			}
			else {
					// This is needed on older Android platforms as the NsdManager does not have it
					// built in before Android 14.
					android.net.wifi.WifiManager wifi = (android.net.wifi.WifiManager) getApplicationContext().getSystemService(android.content.Context.WIFI_SERVICE);
					assert wifi != null;
					multicastLock = wifi.createMulticastLock(TAG);
					multicastLock.setReferenceCounted(true);
					multicastLock.acquire();


					discoveryListener = new NsdManager.DiscoveryListener() {
						@Override
						public void onDiscoveryStarted(String regType) {
							Log.d(TAG, "Discovery started for " + regType);
							try{
								callback.onMDNSstartupCompleted(true);
							}
							catch(NullPointerException e) {
								Log.d(TAG, "callback is NULL, not notified about startup");
							}
						}

						@Override
						public void onServiceFound(NsdServiceInfo service) {
							// A service was found! Do something with it.
							Log.d(TAG, "Discovery found: " + service);
							Message m = Message.obtain(workerThread.handler, MDNSWorkerThread.MESSAGE_RESOLVE);
							m.obj = service;
							m.sendToTarget();
						}

						@Override
						public void onServiceLost(NsdServiceInfo service) {
							connections_discovered.remove(service.getServiceName());

							Log.d(TAG, "Discovery lost: " + service.getServiceName()
									+ ", now " + connections_discovered.size());

							mDNSnotify(service.getServiceName(), null);
						}

						@Override
						public void onDiscoveryStopped(String serviceType) {
							Log.d(TAG, "Discovery stopped: " + serviceType);
						}

						@Override
						public void onStartDiscoveryFailed(String serviceType, int errorCode) {
							Log.e(TAG, "Discovery start failed with error code: " + errorCode);
							try{
								callback.onMDNSstartupCompleted(false);
							}
							catch(NullPointerException e) {
								Log.d(TAG, "callback is NULL, not notified about startup");
							}
						}

						@Override
						public void onStopDiscoveryFailed(String serviceType, int errorCode) {
							Log.e(TAG, "Discovery stop failed with error code: " + errorCode);
						}
					};

					nsdManager.discoverServices(mdnstype, NsdManager.PROTOCOL_DNS_SD, discoveryListener);
			}
		}

		private void mDNSstop()
		{
			Log.d(TAG, "stopping MDNS");
			if (discoveryListener != null) {
				try {
					nsdManager.stopServiceDiscovery(discoveryListener);
				} catch (Exception ignored) {
				}
				discoveryListener = null;
			}

			if(multicastLock != null) {
				multicastLock.release();
				multicastLock = null;
			}

			// notify our callback about our internal state, i.e. the removals
			for(ConnectionBean c: connections_discovered.values())
				mDNSnotify(c.nickname, null);
			// and clear internal state
			connections_discovered.clear();

			Log.d(TAG, "stopping MDNS done");
		}

		// do the GUI stuff in Runnable posted to main thread handler
		private void mDNSnotify(final String conn_name, final ConnectionBean conn) {
			if(callback!=null)
				callback.onMDNSnotify(conn_name, conn, new Hashtable<>(connections_discovered));
			else
				Log.d(TAG, "callback is NULL, not notifying");

		}


	}


}
