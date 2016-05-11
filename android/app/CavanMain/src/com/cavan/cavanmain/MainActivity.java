package com.cavan.cavanmain;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

import android.os.Build;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;

import com.cavan.cavanutils.CavanUtils;

public class MainActivity extends PreferenceActivity {

	private static final String KEY_IP_ADDRESS = "ip_address";

	private File mFileBin;
	private Preference mPreferenceIpAddress;

	@SuppressWarnings("deprecation")
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.cavan_service);

		mPreferenceIpAddress = findPreference(KEY_IP_ADDRESS);
		updateIpAddressStatus();

		mFileBin = getDir("bin", 0777);
		if (mFileBin == null) {
			CavanUtils.logE("Failed to getDir bin");
		} else {
			CavanUtils.appendPathEnv(mFileBin.getPath());

			new Thread() {

				@Override
				public void run() {
					CavanUtils.logD("releaseCavanMain " + (releaseCavanMain() ? "OK" : "Failed"));
				}
			}.start();
		}
	}

	@SuppressWarnings("deprecation")
	private boolean releaseCavanMain() {
		File fileCavanMain = new File(mFileBin, "cavan-main");
		if (fileCavanMain.canExecute()) {
			return true;
		}

		for (String abi : new String[] { Build.CPU_ABI, Build.CPU_ABI2 }) {
			String filename = "cavan-main" + "." + abi;

			if (releaseAsset(filename, fileCavanMain)) {
				fileCavanMain.setExecutable(true, false);
				return true;
			}
		}

		return false;
	}

	private boolean releaseAsset(InputStream inStream, OutputStream outStream) {
		byte[] buff = new byte[1024];

		try {
			while (true) {
				int length = inStream.read(buff);
				if (length < 1) {
					break;
				}

				outStream.write(buff, 0, length);
			}

			return true;
		} catch (Exception e) {
			e.printStackTrace();
			return false;
		}
	}

	private boolean releaseAsset(String filename, File outFile) {
		CavanUtils.logD("releaseAsset: " + filename + " => " + outFile.getPath());

		InputStream inStream = null;
		OutputStream outStream = null;

		try {
			inStream = getAssets().open(filename);
			outStream = new FileOutputStream(outFile);
			return releaseAsset(inStream, outStream);
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (inStream != null) {
				try {
					inStream.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}

			if (outStream != null) {
				try {
					outStream.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}

		return false;
	}

	private void updateIpAddressStatus() {
		Enumeration<NetworkInterface> enNetIf;
		try {
			enNetIf = NetworkInterface.getNetworkInterfaces();
			if (enNetIf == null) {
				return;
			}
		} catch (SocketException e) {
			e.printStackTrace();
			return;
		}

		StringBuilder builder = new StringBuilder();

		while (enNetIf.hasMoreElements()) {
			Enumeration<InetAddress> enAddr = enNetIf.nextElement().getInetAddresses();
			while (enAddr.hasMoreElements()) {
				InetAddress addr = enAddr.nextElement();
				if (addr.isLoopbackAddress()) {
					continue;
				}

				if (addr.isLinkLocalAddress()) {
					continue;
				}

				if (builder.length() > 0) {
					builder.append("\n");
				}

				builder.append(addr.getHostAddress());
			}
		}

		if (builder.length() > 0) {
			mPreferenceIpAddress.setSummary(builder.toString());
		} else {
			mPreferenceIpAddress.setSummary(R.string.text_unknown);
		}
	}

	@SuppressWarnings("deprecation")
	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
		if (preference == mPreferenceIpAddress) {
			updateIpAddressStatus();
		}

		return super.onPreferenceTreeClick(preferenceScreen, preference);
	}
}
