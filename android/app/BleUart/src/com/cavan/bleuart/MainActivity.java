package com.cavan.bleuart;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothAdapter.LeScanCallback;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListAdapter;
import android.widget.ListView;

import com.cavan.cavanutils.CavanUtils;

@SuppressLint("HandlerLeak")
public class MainActivity extends Activity implements OnClickListener, LeScanCallback {

    public static final UUID SERVICE_UUID	= UUID.fromString("0783b03e-8535-b5a0-7140-a304d2495cb7");
    public static final UUID RX_UUID		= UUID.fromString("0783b03e-8535-b5a0-7140-a304d2495cba");
    public static final UUID TX_UUID		= UUID.fromString("0783b03e-8535-b5a0-7140-a304d2495cb8");

	private BluetoothManager mBluetoothManager;
	private BluetoothAdapter mBluetoothAdapter;

	private BluetoothGatt mBluetoothGatt;
	private BluetoothGattService mGattService;
	private BluetoothGattCharacteristic mCharacteristicTx;
	private BluetoothGattCharacteristic mCharacteristicRx;

	private boolean mScanning;
	private Button mButtonScan;
	private ListView mListViewDevices;
	private HashMap<String, MyBluetoothDevice> mHashMapDevices = new HashMap<String, MyBluetoothDevice>();
	private List<MyBluetoothDevice> mListDevices = new ArrayList<MyBluetoothDevice>();
	private DeviceAdapter mDeviceAdapter = new DeviceAdapter();

	private Handler mHandler = new Handler() {

		@Override
		public void handleMessage(Message msg) {
			MyBluetoothDevice device = (MyBluetoothDevice) msg.obj;
			device.updateTextSync();
		}
	};

	class MyBluetoothDevice implements OnClickListener {
		private int mIndex;
		private int mRssi;
		private BluetoothDevice mDevice;
		private boolean mConnected;
		private boolean mDiscovered;
		private BluetoothGattCallback mBluetoothGattCallback = new BluetoothGattCallback() {

			@Override
			public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
				CavanUtils.logE("onConnectionStateChange: " + status + " => " + newState);

				switch (newState) {
				case BluetoothProfile.STATE_CONNECTED:
					mConnected = true;
					mDiscovered = false;
					gatt.discoverServices();
					break;

				case BluetoothProfile.STATE_DISCONNECTED:
					mConnected = false;
					closeGatt();
					break;
				}

				updateText();
			}

			@Override
			public void onServicesDiscovered(BluetoothGatt gatt, int status) {
				CavanUtils.logE("onServicesDiscovered: status = " + status);

				mDiscovered = true;
				updateText();

				int serviceIndex = 0;

				for (BluetoothGattService service : mBluetoothGatt.getServices()) {
					CavanUtils.logE((++serviceIndex) + ". service = " + service.getUuid());

					int characteristicIndex = 0;

					for (BluetoothGattCharacteristic characteristic : service.getCharacteristics()) {
						CavanUtils.logE("\t" + (++characteristicIndex) + ". characteristic = " + characteristic.getUuid());

						int descriptorIndex = 0;

						for (BluetoothGattDescriptor descriptor : characteristic.getDescriptors()) {
							CavanUtils.logE("\t\t" + (++descriptorIndex) + " .descriptor = " + descriptor.getUuid());
						}
					}
				}

				mGattService = mBluetoothGatt.getService(SERVICE_UUID);
				mCharacteristicTx = mGattService.getCharacteristic(TX_UUID);
				mCharacteristicRx = mGattService.getCharacteristic(RX_UUID);

				mBluetoothGatt.setCharacteristicNotification(mCharacteristicTx, true);

				super.onServicesDiscovered(gatt, status);
			}

			@Override
			public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
				CavanUtils.logE("onCharacteristicRead: characteristic = " + characteristic + ", status = " + status);
				super.onCharacteristicRead(gatt, characteristic, status);
			}

			@Override
			public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
				CavanUtils.logE("onCharacteristicWrite: characteristic = " + characteristic + ", status = " + status);
				super.onCharacteristicWrite(gatt, characteristic, status);
			}

			@Override
			public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
				CavanUtils.logE("onCharacteristicChanged: text = " + new String(characteristic.getValue()));
				super.onCharacteristicChanged(gatt, characteristic);
			}

			@Override
			public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
				CavanUtils.logE("onDescriptorRead: descriptor = " + descriptor + ", status = " + status);
				super.onDescriptorRead(gatt, descriptor, status);
			}

			@Override
			public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
				CavanUtils.logE("onDescriptorWrite: descriptor = " + descriptor + ", status = " + status);
				super.onDescriptorWrite(gatt, descriptor, status);
			}

			@Override
			public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
				CavanUtils.logE("onReliableWriteCompleted: " + ", status = " + status);
				super.onReliableWriteCompleted(gatt, status);
			}

			@Override
			public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
				CavanUtils.logE("onReadRemoteRssi: rssi = " + rssi + ", status = " + status);
				super.onReadRemoteRssi(gatt, rssi, status);
			}
		};

		public MyBluetoothDevice(BluetoothDevice device, int rssi) {
			super();
			mRssi = rssi;
			mDevice = device;
		}

		public Button createView(View convertView, Context context, int index) {
			Button button;

			mIndex = index;

			if (convertView != null) {
				button = (Button) convertView;
			} else {
				button = new Button(context);
			}

			updateText(button);
			button.setOnClickListener(this);

			return button;
		}

		public void updateText(Button button) {
			CavanUtils.logE("mConnected = " + mConnected + ", mDiscovered = " + mDiscovered);

			int color;

			if (mConnected) {
				if (mDiscovered) {
					color = Color.GREEN;
				} else {
					color = Color.BLUE;
				}
			} else {
				color = Color.RED;
			}

			button.setTextColor(color);
			button.setText(toString());
		}

		public void updateTextSync() {
			Button button = (Button) mListViewDevices.getChildAt(mIndex);
			if (button != null) {
				updateText(button);
			}
		}

		public void updateText() {
			Message message = mHandler.obtainMessage(0, this);
			message.sendToTarget();
		}

		public void setRssi(int rssi) {
			mRssi = rssi;
			updateText();
		}

		public BluetoothGatt connectGatt(Context context) {
			closeGatt();
			setScanState(false);

			return mDevice.connectGatt(context, false, mBluetoothGattCallback);
		}

		@Override
		public String toString() {
			StringBuilder builder = new StringBuilder(mDevice.getAddress());
			String name = mDevice.getName();
			if (name != null) {
				builder.append(" - ");
				builder.append(name);
			}

			switch (mDevice.getBondState()) {
			case BluetoothDevice.BOND_BONDED:
				builder.append(" - ");
				builder.append("BONDED");
				break;

			case BluetoothDevice.BOND_BONDING:
				builder.append(" - ");
				builder.append("BONDING");
				break;
			}

			builder.append(", RSSI = " + mRssi);

			return builder.toString();
		}

		@Override
		public void onClick(View v) {
			mBluetoothGatt = connectGatt(MainActivity.this);
		}
	}

	class DeviceAdapter extends BaseAdapter {

		@Override
		public int getCount() {
			return mListDevices.size();
		}

		@Override
		public Object getItem(int position) {
			return null;
		}

		@Override
		public long getItemId(int position) {
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			MyBluetoothDevice device = mListDevices.get(position);
			if (device == null) {
				return null;
			}

			return device.createView(convertView, getApplicationContext(), position);
		}

		public void updateDeviceList() {
			mListDevices.clear();

			for (MyBluetoothDevice device : mHashMapDevices.values()) {
				mListDevices.add(device);
			}

			notifyDataSetChanged();
		}
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		mBluetoothManager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
		mBluetoothAdapter = mBluetoothManager.getAdapter();

		mButtonScan = (Button) findViewById(R.id.buttonScan);
		mButtonScan.setOnClickListener(this);

		mListViewDevices = (ListView) findViewById(R.id.listViewDevices);
		mListViewDevices.setAdapter((ListAdapter) mDeviceAdapter);
	}

	public void closeGatt() {
		if (mBluetoothGatt != null) {
			mBluetoothGatt.disconnect();
			mBluetoothGatt.close();
			mBluetoothGatt = null;
		}
	}

	@SuppressWarnings("deprecation")
	private void setScanState(boolean enable) {
		if (enable) {
			mBluetoothAdapter.enable();
			mHashMapDevices.clear();

			closeGatt();

			for (BluetoothDevice device : mBluetoothAdapter.getBondedDevices()) {
				mHashMapDevices.put(device.getAddress(), new MyBluetoothDevice(device, 0));
			}

			mDeviceAdapter.updateDeviceList();

			mBluetoothAdapter.startLeScan(this);
			mButtonScan.setText(R.string.text_scan_stop);

			mHandler.postDelayed(new Runnable() {

				@Override
				public void run() {
					setScanState(false);
				}
			}, 10 * 1000);
		} else {
			mBluetoothAdapter.stopLeScan(this);
			mButtonScan.setText(R.string.text_scan_start);
		}

		mScanning = enable;
	}

	private void switchScanState() {
		setScanState(!mScanning);
	}

	@Override
	public void onClick(View v) {
		switchScanState();
	}

	@Override
	public void onLeScan(BluetoothDevice device, int rssi, byte[] scanRecord) {
		String address = device.getAddress();
		MyBluetoothDevice myDevice = mHashMapDevices.get(address);
		if (myDevice == null) {
			myDevice = new MyBluetoothDevice(device, rssi);
			mHashMapDevices.put(address, myDevice);
			mDeviceAdapter.updateDeviceList();
		} else {
			myDevice.setRssi(rssi);
		}
	}
}
