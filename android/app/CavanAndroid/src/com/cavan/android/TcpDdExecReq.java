package com.cavan.android;

import com.cavan.java.ByteCache;

class TcpDdExecReq extends TcpDdPackage {
	private short mLines;
	private short mColumns;
	private String mCommand;

	public TcpDdExecReq(int lines, int columns, String command) {
		super(TcpDdClient.TCP_DD_EXEC);

		mLines = (short) lines;
		mColumns = (short) columns;
		mCommand = command;
	}

	public TcpDdExecReq(String command) {
		this((short) -1, (short) -1, command);
	}

	public TcpDdExecReq() {
		this("shell");
	}

	@Override
	public String toString() {
		return String.format("lines = %d, columns = %d, command = %s", mLines, mColumns, mCommand);
	}

	@Override
	protected boolean encodeBody(ByteCache cache) {
		if (!cache.writeValue16(mLines)) {
			CavanAndroid.logE("Failed to writeValue16(mLines)");
			return false;
		}

		if (!cache.writeValue16(mColumns)) {
			CavanAndroid.logE("Failed to writeValue16(mColumns)");
			return false;
		}

		if (!cache.writeBytes(mCommand.getBytes())) {
			CavanAndroid.logE("Failed to writeBytes(mCommand)");
			return false;
		}

		return true;
	}

	@Override
	public byte[] encode() {
		return encode(8 + mCommand.getBytes().length);
	}
}