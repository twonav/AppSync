package org.bluetooth.TwoNavSync;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * Created by luigi on 25/10/17.
 */

public class TwoNavBootReceiver extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent)
    {
        if(!TwoNavSyncService.isMyServiceRunning(context,TwoNavSyncService.class))
        {
            Intent startServiceIntent = new Intent(context, TwoNavSyncService.class);
            context.startService(startServiceIntent);
        }
    }
}
