package com.cavan.java;

public class CavanPeakValleyValue {

	public static final int TYPE_FALLING = 0;
	public static final int TYPE_RISING = 1;

	protected int mType;
	protected long mTime;
	protected double mPeakValue;
	protected double mValleyValue;

	public CavanPeakValleyValue(double peak, double valley, int type, long time) {
		super();

		mType = type;
		mTime = time;
		mPeakValue = peak;
		mValleyValue = valley;
	}

	public CavanPeakValleyValue(double peak, double valley, int type) {
		this(peak, valley, type, System.currentTimeMillis());
	}

	public CavanPeakValleyValue(double peak, double valley) {
		this(peak, valley, TYPE_FALLING);
	}

	public CavanPeakValleyValue(double value) {
		this(value, value);
	}

	public double getPeakValue() {
		return mPeakValue;
	}

	public void setPeakValue(double peak) {
		mPeakValue = peak;
	}

	public double getValleyValue() {
		return mValleyValue;
	}

	public void setValleyValue(double valley) {
		mValleyValue = valley;
	}

	public int getType() {
		return mType;
	}

	public void setType(int type) {
		mType = type;
	}

	public void setTime(long time) {
		mTime = time;
	}

	public long getTime() {
		return mTime;
	}

	public boolean isFalling() {
		return mType == TYPE_FALLING;
	}

	public boolean isRising() {
		return mType == TYPE_RISING;
	}

	public double getRange() {
		return mPeakValue - mValleyValue;
	}

	public double getBaseline() {
		return (mPeakValue + mValleyValue) / 2;
	}

	public void extend(CavanPeakValleyValue value) {
		if (value.getPeakValue() > mPeakValue) {
			mPeakValue = value.getPeakValue();
		}

		if (value.getValleyValue() < mValleyValue) {
			mValleyValue = value.getValleyValue();
		}
	}

	public CavanPeakValleyValue copyPeakValley() {
		return new CavanPeakValleyValue(mPeakValue, mValleyValue, mType, mTime);
	}

	@Override
	public String toString() {
		if (isRising()) {
			return String.format("[%6.2f, %6.2f]", mValleyValue, mPeakValue);
		} else {
			return String.format("[%6.2f, %6.2f]", mPeakValue, mValleyValue);
		}
	}
}
