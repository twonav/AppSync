package org.bluetooth.TwoNavSync;

import android.app.ActivityManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;

/**
 * Created by luigi on 20/10/17.
 */

public class TwoNavSyncService extends Service
{
    private Looper mServiceLooper;
    private ServiceHandler mServiceHandler;

    TwoNavGattServer mGattServer;

    public static boolean isMyServiceRunning(Context context, Class<?> serviceClass) {
        ActivityManager manager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }

    // Handler that receives messages from the thread
    private final class ServiceHandler extends Handler
    {
        public ServiceHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg)
        {
            // Normally we would do some work here, like download a file.
            // For our sample, we just sleep for 5 seconds.
            try {
                Thread.sleep(5000);
            } catch (InterruptedException e) {
                // Restore interrupt status.
                Thread.currentThread().interrupt();
            }
            // Stop the service using the startId, so that we don't stop
            // the service in the middle of handling another job
            stopSelf(msg.arg1);
        }
    }

    @Override
    public void onCreate()
    {
        Intent notifications = new Intent(this,NotificationService.class);
        notifications.putExtra("KEY_DUMMY","Dummy Value");
        this.startService(notifications);

        /*HandlerThread thread = new HandlerThread("ServiceStartArgs", Process.THREAD_PRIORITY_BACKGROUND);
        thread.start();

        mServiceLooper = thread.getLooper();
        mServiceHandler = new ServiceHandler(mServiceLooper);

        Log.v("Ble","TwoNavSyncService Was Started");*/
        mGattServer = new TwoNavGattServer(this);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        //TODO do something useful
        return Service.START_STICKY;
    }

    @Override
    public void onDestroy()
    {

    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
