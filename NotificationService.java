package org.bluetooth.TwoNavSync;


import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.os.IBinder;

/**
 * Created by mukesh on 19/5/15.
 */
public class NotificationService extends NotificationListenerService {

    Context context;

    @Override
    public void onCreate() {

        super.onCreate();
        context = getApplicationContext();
        Log.i("notifications","notifications service created");
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }


    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        String pack = sbn.getPackageName();

        Bundle extras = sbn.getNotification().extras;
        String title = extras.getString("android.title");
        String text = extras.getCharSequence("android.text").toString();


        Log.i("Package",pack);
        Log.i("Title",title);
        Log.i("Text",text);
        Log.i("Flags","Added");

        Intent msgrcv = new Intent("NotificationMsg");
        msgrcv.putExtra("id", sbn.getId());
        msgrcv.putExtra("package", pack);
        msgrcv.putExtra("title", title);
        msgrcv.putExtra("text", text);
        msgrcv.putExtra("flags", 0x01);
        msgrcv.putExtra("time", sbn.getPostTime());


        LocalBroadcastManager.getInstance(context).sendBroadcast(msgrcv);
    }

    @Override

    public void onNotificationRemoved(StatusBarNotification sbn) {
        Log.i("Msg","Notification Removed");

        Log.i("flags","Removed");

        Intent msgrcv = new Intent("NotificationMsg");
        msgrcv.putExtra("flags", 0x02);
        msgrcv.putExtra("id", sbn.getId());

        LocalBroadcastManager.getInstance(context).sendBroadcast(msgrcv);
    }

}