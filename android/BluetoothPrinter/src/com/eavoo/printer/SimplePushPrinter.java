package com.eavoo.printer;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import javax.obex.ClientOperation;
import javax.obex.ClientSession;
import javax.obex.HeaderSet;
import javax.obex.ResponseCodes;

import android.content.Context;
import android.os.PowerManager;
import android.os.Process;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.webkit.MimeTypeMap;

public class SimplePushPrinter extends Thread
{
    private static final String TAG = "Bpp ObexClient";
	private WakeLock mWakeLock;
	private Context mContext;
	private BppObexTransport mTransport;
	private ClientSession mObexClientSession;
	private String mFilePathName = "/mnt/sdcard/test.jpg";
	private String mFileMimeType = null;

	private String ByteArrayToHexString(byte[] bs)
	{
		if (bs == null)
		{
			return "";
		}

		StringBuilder stringBuilder = new StringBuilder();

		for (byte b : bs)
		{
			if ((b & 0xF0) == 0)
			{
				stringBuilder.append("0");
			}

			stringBuilder.append(Integer.toHexString((b >> 4) & 0x0F) + Integer.toHexString(b & 0x0F));
		}

		return stringBuilder.toString();
	}

	private void CavanLog(String message)
	{
		Log.v("Cavan SimplePushPrinter", "\033[1m" + message + "\033[0m");
	}

	private void CavanLog(byte[] bs)
	{
		CavanLog(ByteArrayToHexString(bs));
	}

	public SimplePushPrinter(Context context, BppObexTransport transport)
	{
		CavanLog("Create PrintThread");
		PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
		this.mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);

		this.mContext = context;
		this.mTransport = transport;
	}

	private boolean connect()
	{
		CavanLog("Create ClientSession with transport " + mTransport.toString());

		try
		{
			CavanLog("Connect to printer");
			mTransport.connect();
			CavanLog("Create OBEX client session");
			mObexClientSession = new ClientSession(mTransport);
		}
		catch (IOException e1)
		{
			CavanLog("OBEX session create error");
			return false;
		}

		HeaderSet hsRequest = new HeaderSet();

		hsRequest.setHeader(HeaderSet.TARGET, BppObexTransport.UUID_DPS);

		try
		{
			Log.d(TAG, "Connect to OBEX session");
			HeaderSet hsResponse = mObexClientSession.connect(hsRequest);
			CavanLog("ResponseCode = " + hsResponse.getResponseCode());

			byte[] headerWho = (byte[]) hsResponse.getHeader(HeaderSet.WHO);
			if (headerWho != null)
			{
				CavanLog("HeaderWho:");
				CavanLog(headerWho);
			}

			if (hsResponse.mConnectionID == null)
			{
				CavanLog("mConnectionID == null");
			}
			else
			{
				CavanLog(hsResponse.mConnectionID);
			}

			if (hsResponse.getResponseCode() == ResponseCodes.OBEX_HTTP_OK)
			{
				return true;
			}
		} catch (IOException e) {
			CavanLog("OBEX session connect error");
		}

		try {
			mTransport.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return false;
	}

	private void disconnect()
	{
		CavanLog("disconnect");

		if (mObexClientSession != null)
		{
			try {
				mObexClientSession.disconnect(null);
				mObexClientSession.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

		if (mTransport != null)
		{
			try {
				mTransport.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}

	private String GetFileMimeTypeByName(String pathname)
	{
            String extension, type;

            int dotIndex = pathname.lastIndexOf(".");
            if (dotIndex < 0)
            {
            	extension = "txt";
            }
            else
            {
            	extension = pathname.substring(dotIndex + 1).toLowerCase();
            }

            MimeTypeMap map = MimeTypeMap.getSingleton();

            return map.getMimeTypeFromExtension(extension);
	}

	private boolean PrintContent()
	{
		File file = new File(mFilePathName);
		long fileLength = file.length();

		if (fileLength == 0)
		{
			CavanLog("File " + mFilePathName + " don't exist");
			return false;
		}

		HeaderSet reqHeaderSet = new HeaderSet();

		int index = mFilePathName.lastIndexOf('/');
		if (index < 0)
		{
			reqHeaderSet.setHeader(HeaderSet.NAME, mFilePathName);
		}
		else
		{
			reqHeaderSet.setHeader(HeaderSet.NAME, mFilePathName.substring(index + 1));
		}

		if (mFileMimeType == null)
		{
			mFileMimeType = GetFileMimeTypeByName(mFilePathName);
		}

		CavanLog("mimetype = " + mFileMimeType);

		reqHeaderSet.setHeader(HeaderSet.TYPE, "mFileMimeType");
		reqHeaderSet.setHeader(HeaderSet.LENGTH, fileLength);

		ClientOperation clientOperation;
		try {
			clientOperation = (ClientOperation) mObexClientSession.put(reqHeaderSet);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			clientOperation = null;
		}

		if (clientOperation == null)
		{
			CavanLog("clientOperation == null");
			return false;
		}

		OutputStream obexOutputStream;
		try {
			obexOutputStream = clientOperation.openOutputStream();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			obexOutputStream = null;
		}

		if (obexOutputStream == null)
		{
			CavanLog("obexOutputStream == null");
			try {
				clientOperation.abort();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			return false;
		}

		InputStream obexInputStream;
		try {
			obexInputStream = clientOperation.openInputStream();
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
			obexInputStream = null;
		}

		if (obexInputStream == null)
		{
			CavanLog("obexInputStream == null");

			try {
				obexOutputStream.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			try {
				clientOperation.abort();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			return false;
		}

		FileInputStream fileInputStream;
		try {
			fileInputStream = new FileInputStream(file);
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			fileInputStream = null;
		}

		if (fileInputStream == null)
		{
			CavanLog("fileInputStream == null");

			try {
				obexInputStream.close();
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}

			try {
				obexOutputStream.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			try {
				clientOperation.abort();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			return false;
		}

		BufferedInputStream bufferedInputStream = new BufferedInputStream(fileInputStream);
		int obexMaxPackageSize = clientOperation.getMaxPacketSize();
		byte[] buff = new byte[obexMaxPackageSize];

		boolean boolResult = true;

		CavanLog("Start send file");

		while (fileLength != 0)
		{
			int sendLength;

			try {
				sendLength = bufferedInputStream.read(buff);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				sendLength = -1;
			}

			CavanLog("readLength = " + sendLength);

			if (sendLength < 0)
			{
				boolResult = false;
				break;
			}

			try {
				obexOutputStream.write(buff, 0, sendLength);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				boolResult = false;
				break;
			}

			int responseCode;
			try {
				responseCode = clientOperation.getResponseCode();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				boolResult = false;
				break;
			}

			CavanLog("responseCode = " + responseCode);

			if (responseCode != ResponseCodes.OBEX_HTTP_CONTINUE && responseCode != ResponseCodes.OBEX_HTTP_OK)
			{
				CavanLog("responseCode != ResponseCodes.OBEX_HTTP_CONTINUE && responseCode != ResponseCodes.OBEX_HTTP_OK");
				boolResult = false;
				break;
			}

			fileLength -= sendLength;

			CavanLog("sendLength = " + sendLength + ", fileLength = " + fileLength);
		}

		try {
			bufferedInputStream.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		try {
			obexInputStream.close();
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}

		try {
			fileInputStream.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		try {
			obexOutputStream.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		try {
			if (boolResult)
			{
				clientOperation.close();
			}
			else
			{
				clientOperation.abort();
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return boolResult;
	}

	@Override
	public void run()
	{
		Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);

		CavanLog("Printer thread running");

		mWakeLock.acquire();

		if (connect())
		{
			CavanLog("Connect successfully");
		}
		else
		{
			CavanLog("Connect failed");
			return;
		}

		if (PrintContent())
		{
			CavanLog("Print complete");
		}
		else
		{
			CavanLog("Print failed");
		}

		disconnect();

		mWakeLock.release();
	}
}
