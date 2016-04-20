package com.cavan.cavanutils;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;

public class CavanTcpClient extends CavanNetworkClient {

	private int mPort;
	private Socket mSocket;
	private InetAddress mSocketAddress;

	public CavanTcpClient(InetAddress address, int port) {
		mSocketAddress = address;
		mPort = port;
	}

	@Override
	protected boolean openSocket() {
		try {
			mSocket = new Socket(mSocketAddress, mPort);
			mSocket.setTcpNoDelay(true);
			return true;
		} catch (IOException e) {
			e.printStackTrace();
		}

		mSocket = null;

		return false;
	}

	@Override
	protected void closeSocket() {
		if (mSocket == null) {
			return;
		}

		try {
			mSocket.shutdownInput();
		} catch (IOException e) {
			// e.printStackTrace();
		}

		try {
			mSocket.shutdownOutput();
		} catch (IOException e) {
			// e.printStackTrace();
		}

		try {
			mSocket.close();
		} catch (IOException e) {
			// e.printStackTrace();
		}

		mSocket = null;
	}

	@Override
	protected InputStream getInputStream() {
		try {
			return mSocket.getInputStream();
		} catch (IOException e) {
			e.printStackTrace();
		}

		return null;
	}

	@Override
	protected OutputStream getOutputStream() {
		try {
			return mSocket.getOutputStream();
		} catch (IOException e) {
			e.printStackTrace();
		}

		return null;
	}
}
