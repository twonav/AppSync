package org.bluetooth.TwoNavSync;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Environment;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;


/**
 * Created by luigi on 20/10/17.
 */

public class TwoNavGattServer {
    protected static final String TAG = "Ble Devices";

    public static final UUID TWONAV_SYNC_SERVICE_UUID = UUID.fromString("A3EF4942-C8FC-11E5-A837-0800200C9A66");
    public static final UUID TWONAV_SYNC_NOTIFICATION_CHARACTERISTIC_UUID = UUID.fromString("46df2ad2-c827-47ae-9932-b64c3da77adc");
    public static final UUID TWONAV_SYNC_WRITE_CHARACTERISTIC_UUID = UUID.fromString("2a226f3d-d38f-41ad-8692-5ee97386b2ee");
    public static final UUID TWONAV_SYNC_FT_NOTIFICATION_CHARACTERISTIC_UUID = UUID.fromString("3cf4d6f3-3fae-4fe3-846c-5b5b1286ac0c");
    public static final UUID TWONAV_SYNC_FT_WRITE_CHARACTERISTIC_UUID = UUID.fromString("61e63c0f-3f4e-42b4-afac-70175d919543");

    public static final UUID DESCRIPTOR_CLIENT_CONFIG_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");// DESCRIPTOR

    private BluetoothGattServer mGattServer = null;
    private BluetoothGatt mGatt = null;
    private GattServerBroadcastReceiver mReceiver = null;
    private BluetoothManager mBluetoothManager;
    private Context mContext;

    private BluetoothDevice mDevice;
    private int mtuMax = 27;
    public Semaphore sem;


    public TwoNavGattServer(Context context) {
        mBluetoothManager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        mContext = context;
        initReceiver();
        sem = new Semaphore(0);
    }

    private void initReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction("NotificationMsg");
        mReceiver = new GattServerBroadcastReceiver();
        LocalBroadcastManager manager = LocalBroadcastManager.getInstance(mContext);
        manager.registerReceiver(mReceiver, filter);
        InitGattServer();
    }

    private BluetoothGattService CreateCompacketService() {
        BluetoothGattService service = new BluetoothGattService(TWONAV_SYNC_SERVICE_UUID, BluetoothGattService.SERVICE_TYPE_PRIMARY);
        if (service != null) {
            // create characteristics
            BluetoothGattCharacteristic notificationCharacteristic = new BluetoothGattCharacteristic(TWONAV_SYNC_NOTIFICATION_CHARACTERISTIC_UUID,
                    BluetoothGattCharacteristic.PROPERTY_NOTIFY | BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE,
                    BluetoothGattCharacteristic.PERMISSION_READ_ENCRYPTED_MITM | BluetoothGattCharacteristic.PERMISSION_WRITE_ENCRYPTED_MITM);

            BluetoothGattDescriptor notifyDescriptor = new BluetoothGattDescriptor(DESCRIPTOR_CLIENT_CONFIG_UUID, BluetoothGattDescriptor.PERMISSION_WRITE_ENCRYPTED_MITM);
            notifyDescriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            notificationCharacteristic.addDescriptor(notifyDescriptor);

            BluetoothGattCharacteristic writeCharacteristic = new BluetoothGattCharacteristic(TWONAV_SYNC_WRITE_CHARACTERISTIC_UUID,
                    BluetoothGattCharacteristic.PROPERTY_WRITE,
                    BluetoothGattCharacteristic.PERMISSION_WRITE_ENCRYPTED_MITM);

            /*BluetoothGattCharacteristic notificationFtCharacteristic = new BluetoothGattCharacteristic(TWONAV_SYNC_FT_NOTIFICATION_CHARACTERISTIC_UUID,
                    BluetoothGattCharacteristic.PROPERTY_NOTIFY,
                    BluetoothGattCharacteristic.PERMISSION_READ_ENCRYPTED_MITM| BluetoothGattCharacteristic.PERMISSION_WRITE_ENCRYPTED_MITM);

            BluetoothGattCharacteristic writeFtCharacteristic = new BluetoothGattCharacteristic(TWONAV_SYNC_FT_WRITE_CHARACTERISTIC_UUID,
                    BluetoothGattCharacteristic.PROPERTY_WRITE,
                    BluetoothGattCharacteristic.PERMISSION_WRITE_ENCRYPTED_MITM);*/

            service.addCharacteristic(notificationCharacteristic);
            service.addCharacteristic(writeCharacteristic);
            /*service.addCharacteristic(notificationFtCharacteristic);
            service.addCharacteristic(writeFtCharacteristic);*/
        }
        return service;
    }

    // Init compacket service
    private void InitGattServer() {
        if (mBluetoothManager.getAdapter().getState() == BluetoothAdapter.STATE_ON) {
            mGattServer = mBluetoothManager.openGattServer(mContext, mBluetoothGattServerCallback);
            if (mGattServer != null) {
                BluetoothGattService service = CreateCompacketService();
                if (service != null) {
                    mGattServer.addService(service);
                    Log.v("Ble Devices", "GATT Server: Compacket initialized");
                } else {
                    Log.v("Ble Devices", "GATT Server: Compacket was null");
                }
            } else {
                Log.v("Ble Devices", "GATT Server was null");
            }
        }
    }

    private void CloseGattServer() {
        if (mGattServer != null) {
            mGattServer.clearServices();
            mGattServer.close();
            mGattServer = null;
        }
    }

    public BluetoothGattCharacteristic GetNotificationCharacteristic() {
        if (mGattServer != null) {
            BluetoothGattService service = mGattServer.getService(TWONAV_SYNC_SERVICE_UUID);
            if (service != null) {
                return service.getCharacteristic(TWONAV_SYNC_NOTIFICATION_CHARACTERISTIC_UUID);
            }
        }
        return null;
    }

    /*public BluetoothGattCharacteristic GetFtNotificationCharacteristic()
    {
        if(mGattServer!=null)
        {
            BluetoothGattService service = mGattServer.getService(TWONAV_SYNC_SERVICE_UUID);
            if(service!=null)
            {
                return service.getCharacteristic(TWONAV_SYNC_FT_NOTIFICATION_CHARACTERISTIC_UUID);
            }
        }
        return null;
    }*/

    public BluetoothGattServer GetGattServer() {
        return mGattServer;
    }

    public BluetoothGattCharacteristic GetWriteCharacteristic() {
        if (mGattServer != null) {
            BluetoothGattService service = mGattServer.getService(TWONAV_SYNC_SERVICE_UUID);
            if (service != null) {
                return service.getCharacteristic(TWONAV_SYNC_WRITE_CHARACTERISTIC_UUID);
            }
        }
        return null;
    }

    private BluetoothGattServerCallback mBluetoothGattServerCallback = new BluetoothGattServerCallback() {

        @Override
        public void onConnectionStateChange(BluetoothDevice device, int status, int newState) {
            Log.v("Ble Devices", "GATT Server: Compacket state changed: " + status + " " + newState);
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                mDevice = device;
                mDevice.connectGatt(mContext, true, new BluetoothGattCallback() {
                    @Override
                    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
                        if (newState == BluetoothGatt.STATE_CONNECTED) {
                            mGatt = gatt;
                            gatt.requestMtu(1024);
                            gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_BALANCED);
                        }
                        super.onConnectionStateChange(gatt, status, newState);
                    }

                    public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
                        mtuMax = mtu;
                        super.onMtuChanged(gatt, mtu, status);
                    }
                });

                //mGattServer.connect(mDevice, true);
                Log.v("Ble Devices", "GATT Server: CONNECTED");
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.v("Ble Devices", "GATT Server: DISCONNECTED");
            }

        }

        @Override
        public void onServiceAdded(int status, BluetoothGattService service) {
            Log.i("Ble Devices", "GATT Server: Compacket service added: " + service.getUuid().toString());

        }

        @Override
        public void onCharacteristicReadRequest(BluetoothDevice device, int requestId, int offset, BluetoothGattCharacteristic characteristic) {
            Log.i("Ble Devices", "GATT Server: onCharacteristicReadRequest");

        }

        @Override
        public void onCharacteristicWriteRequest(BluetoothDevice device, int requestId, BluetoothGattCharacteristic characteristic,
                                                 boolean preparedWrite, boolean responseNeeded, int offset, byte[] value) {

            Log.i("Ble Devices", "GATT Server: Characteristic write request. Num bytes: " + value.length);
            if (responseNeeded) {
                mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset, value);
            }
            int bytesRead = value.length;
            //TwoNavCommand com = new TwoNavCommand(mTwoNavBT.getContext(),TwoNavCommand.COM_PACKET, value, bytesRead);
            //com.Execute();

        }

        @Override
        public void onDescriptorReadRequest(BluetoothDevice device, int requestId, int offset, BluetoothGattDescriptor descriptor) {
            Log.i("Ble Devices", "GATT Server: onDescriptorReadRequest");

        }

        @Override
        public void onDescriptorWriteRequest(BluetoothDevice device, int requestId, BluetoothGattDescriptor descriptor,
                                             boolean preparedWrite, boolean responseNeeded, int offset, byte[] value) {
            Log.i("Ble Devices", "GATT Server: onDescriptorWriteRequest");
            mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset, value);
        }

        @Override
        public void onExecuteWrite(BluetoothDevice device, int requestId, boolean execute) {
            Log.i("Ble Devices", "GATT Server: onExecuteWrite");
        }

        @Override
        public void onNotificationSent(BluetoothDevice device, int status) {
            Log.i("Ble Devices", "GATT Server: onNotificationSent. Status = " + status);
            super.onNotificationSent(device, status);
            sem.release();
        }

        public void onMtuChanged(BluetoothDevice device, int mtu) {
            Log.i("Ble Devices", "GATT Server: onMtuChanged. mtu = " + mtu);
            mtuMax = mtu;
        }
    };


    public class GattServerBroadcastReceiver extends BroadcastReceiver {
        private boolean AddAttribute(ByteArrayOutputStream outputStream, int id, byte[] attribute) throws IOException {

            if (outputStream == null) {
                throw new IOException();
            }

            int attLength = attribute.length > mtuMax ? mtuMax : attribute.length;

            // App Package
            outputStream.write(id);
            outputStream.write(attLength & 0xFF);
            outputStream.write((attLength >> 8) & 0xFF);
            if (attLength > 0)
                outputStream.write(attribute, 0, attLength);

            return true;
        }


        private boolean FormatNotification(ByteArrayOutputStream outputStream, Intent intent) {
            try {
                outputStream.write(0x00);
                byte flags = (byte) (intent.getIntExtra("flags", 0x00) & 0xFF);
                outputStream.write(flags&0xFF);

                int id = intent.getIntExtra("id", 0);
                outputStream.write(id & 0xFF);
                outputStream.write(((id >> 8) & 0xFF));
                outputStream.write(((id >> 16) & 0xFF));
                outputStream.write(((id >> 24) & 0xFF));

                if(flags == 0x01) {
                    AddAttribute(outputStream, 0x00, intent.getStringExtra("package").getBytes());
                    AddAttribute(outputStream, 0x01, intent.getStringExtra("title").getBytes());
                    AddAttribute(outputStream, 0x02, intent.getStringExtra("text").getBytes());
                }
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
            return true;
        }

        private void TransferFile(String filePath, int chunkSize) {
            mGatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH);
            BluetoothGattCharacteristic characteristic = GetNotificationCharacteristic();
            File file = new File(filePath);
            if (file.exists()) {
                ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                try {
                    BufferedInputStream buf = new BufferedInputStream(new FileInputStream(file));
                    Log.i("NotificationMsg", "Transmission Started: " + buf.available());
                    byte[] bytes = new byte[chunkSize];
                    byte countL = 0;
                    byte countH = 0;
                    while (buf.read(bytes) > 0) {
                        outputStream.write(bytes);
                        bytes[0] = countL;
                        countL++;
                        bytes[1] = countH;
                        if (countL == 0xFF)
                            countH++;

                        characteristic.setValue(bytes);

                        int remaining = buf.available();
                        //boolean wasSent = mGatt.writeCharacteristic(characteristic);
                        boolean wasSent = mGattServer.notifyCharacteristicChanged(mDevice, characteristic, buf.available() == 0);
                        sem.acquire();//wasSent &= sem.tryAcquire(10, TimeUnit.SECONDS);
                        if (wasSent)
                            Log.i("NotificationMsg", "RemainingBytes: " + remaining);
                        else {
                            Log.i("NotificationMsg", "Transmission error");
                            break;
                        }

                    }

                    buf.close();
                } catch (FileNotFoundException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            mGatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_BALANCED);
        }

        private void SendNotification(Intent intent) {
            Log.i("NotificationMsg", "Reveived notification");
            if (mDevice != null) {

                BluetoothGattCharacteristic characteristic = GetNotificationCharacteristic();
                if (characteristic != null) {
                    try {
                        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                        if (FormatNotification(outputStream, intent)) {
                            characteristic.setValue(outputStream.toByteArray());
                            boolean wasSent = mGattServer.notifyCharacteristicChanged(mDevice, characteristic, true);
                            sem.acquire(); //sem.tryAcquire(1, TimeUnit.SECONDS);
                            String strDev = "name= " + mDevice.getName() + " address= " + mDevice.getAddress();
                            Log.i("NotificationMsg", "Notifiaction was " + (wasSent ? "Sent" : "Not Sent") + " to " + strDev);
                        }

                        String text = intent.getStringExtra("text");
                        if (text.contentEquals("testble")) {
                            TransferFile(Environment.getExternalStorageDirectory() + "/" + "Track1.zip", mtuMax - 3);
                        }

                        /*
                        String text = intent.getStringExtra("text");
                        if(text == "testspp") {
                            BlueSPP spp = new BlueSPP(mContext);
                            spp.SendThroughSPP(mDevice.getAddress(), Environment.getExternalStorageDirectory() + "/" + "Track1.trk");
                        }
                        */


                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }

                }
            }
        }

        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            switch (action) {
                //--------------------------------------------------------------------------
                case "NotificationMsg":
                    SendNotification(intent);
                    break;
                default:
                    //mGattServer.close();
                    break;

            }
        }
    }

}



