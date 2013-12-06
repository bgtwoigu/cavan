package com.cavan.huahardwareinfo;

import android.os.Build;

public class HuaCameraInfo {
	private static final HuaCameraInfo[] mCameraInfo_HS8801 = {
		new HuaCameraInfo("AT2250", R.string.vendor_name_hongtu, R.string.vendor_name_sanglaishi),
		new HuaCameraInfo("SP2518", R.string.vendor_name_sibike, R.string.vendor_name_boyi)
	};

	private String mName;
	private int mIcVendor;
	private int mVendorName;

	public HuaCameraInfo(String name, int icVendor, int vendorName) {
		super();
		mName = name;
		mIcVendor = icVendor;
		mVendorName = vendorName;
	}

	public String getName() {
		return mName;
	}

	public int getIcVendor() {
		return mIcVendor;
	}

	public int getVendorName() {
		return mVendorName;
	}

	public static HuaCameraInfo getCameraInfo(String name) {
		HuaCameraInfo[] infos;

		if (Build.BOARD.equals("hs8801")) {
			infos = mCameraInfo_HS8801;
		} else {
			return null;
		}

		name = name.toUpperCase();

		for (HuaCameraInfo info : infos) {
			if (info.getName().equals(name)) {
				return info;
			}
		}

		return null;
	}
}