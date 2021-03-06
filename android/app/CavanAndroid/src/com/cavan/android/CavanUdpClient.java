package com.cavan.android;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;

public class CavanUdpClient extends CavanNetworkClientImpl {

	private MulticastSocket mSocket;
	private InetAddress mAddress;
	private int mPort;

	private InetAddress mRemoteAddress;
	private int mRemotePort;

	public CavanUdpClient(InetAddress address, int port) {
		mAddress = address;
		mPort = port;
	}

	@Override
	public boolean openSocket() {
		try {
			mSocket = new MulticastSocket();
			return true;
		} catch (IOException e) {
			e.printStackTrace();
		}

		return false;
	}

	@Override
	public void closeSocket() {
		if (mSocket == null) {
			return;
		}

		mSocket.close();
		mSocket = null;
	}

	class SocketInputStream extends InputStream {

		@Override
		public int read() throws IOException {
			return 0;
		}

		@Override
		public int read(byte[] bytes, int offset, int count) throws IOException {
			DatagramPacket packet = new DatagramPacket(bytes, offset, count);
			mSocket.receive(packet);

			mRemoteAddress = packet.getAddress();
			mRemotePort = packet.getPort();

			return packet.getLength();
		}
	}

	class SocketOutputStream extends OutputStream {

		@Override
		public void write(int value) throws IOException {
			write(new byte[] { (byte) value });
		}

		@Override
		public void write(byte[] bytes, int offset, int count) throws IOException {
			DatagramPacket packet = new DatagramPacket(bytes, offset, count, mAddress, mPort);
			mSocket.send(packet);
		}
	}

	@Override
	public InputStream getInputStream() {
		return new SocketInputStream();
	}

	@Override
	public OutputStream getOutputStream() {
		return new SocketOutputStream();
	}

	@Override
	public Object getAddress() {
		return mAddress;
	}

	@Override
	public int getPort() {
		return mPort;
	}

	@Override
	public void setAddress(Object address) {
		mAddress = (InetAddress) address;
	}

	@Override
	public void setPort(int port) {
		mPort = port;
	}

	@Override
	public Object getRemoteAddress() {
		return mRemoteAddress;
	}

	@Override
	public int getRemotePort() {
		return mRemotePort;
	}
}
