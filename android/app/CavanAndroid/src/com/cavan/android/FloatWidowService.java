package com.cavan.android;

import java.util.HashMap;

import android.app.Service;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.os.RemoteException;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.LinearLayout;

public abstract class FloatWidowService extends Service {

	protected View mRootView;
	protected ViewGroup mViewGroup;
	protected WindowManager mManager;
	protected HashMap<Integer, View> mViewMapId = new HashMap<Integer, View>();
	protected HashMap<CharSequence, View> mViewMapText = new HashMap<CharSequence, View>();

	private IFloatWindowService.Stub mBinder = new IFloatWindowService.Stub() {

		@Override
		public int addText(CharSequence text, int index) throws RemoteException {
			View view = FloatWidowService.this.addText(text, index);
			if (view != null) {
				return view.getId();
			}

			return -1;
		}

		@Override
		public void removeText(CharSequence text) throws RemoteException {
			FloatWidowService.this.removeText(text);
		}

		@Override
		public void removeTextAt(int index) throws RemoteException {
			FloatWidowService.this.removeTextAt(index);
		}

		@Override
		public void removeTextById(int id) throws RemoteException {
			FloatWidowService.this.removeText(id);

		}

		@Override
		public void removeTextAll() throws RemoteException {
			FloatWidowService.this.removeTextAll();
		}
	};

	// ================================================================================

	protected abstract CharSequence getViewText(View view);
	protected abstract View createView(CharSequence text);

	protected ViewGroup findViewGroup() {
		if (mRootView instanceof ViewGroup) {
			return (ViewGroup) mRootView;
		}

		return null;
	}

	protected boolean doInitialize() {
		return true;
	}

	// ================================================================================

	public ViewGroup getViewGroup() {
		return mViewGroup;
	}

	public View getRootView() {
		return mRootView;
	}

	synchronized public void setViewGroup(ViewGroup group) {
		if (mViewGroup == group) {
			return;
		}

		if (mViewGroup != null) {
			mViewGroup.removeAllViews();
		}

		if (group != null) {
			for (View view : mViewMapId.values()) {
				group.addView(view);
			}
		}

		mViewGroup = group;
	}

	synchronized protected void addView(View view, int id, int index) {
		view.setId(id);

		if (mViewGroup != null) {
			if (index < 0) {
				mViewGroup.addView(view);
			} else {
				mViewGroup.addView(view, index);
			}
		}

		mViewMapId.put(id, view);
	}

	synchronized public int addView(View view, int index) {
		for (int id = 1; id > 0; id++) {
			if (mViewMapId.get(id) == null) {
				addView(view, id, index);
				return id;
			}
		}

		return -1;
	}

	synchronized public View findView(int id) {
		return mViewMapId.get(id);
	}

	synchronized public View findView(CharSequence text) {
		for (View view : mViewMapId.values()) {
			if (text.equals(getViewText(view))) {
				return view;
			}
		}

		return null;
	}

	synchronized public View getViewAt(int index) {
		if (mViewGroup == null) {
			return null;
		}

		try {
			return mViewGroup.getChildAt(index);
		} catch (Exception e) {
			return null;
		}
	}

	synchronized public void removeView(View view) {
		if (mViewGroup != null) {
			mViewGroup.removeView(view);
		}

		mViewMapId.remove(view.getId());
	}

	synchronized public View removeView(int id) {
		View view = findView(id);
		if (view != null) {
			removeView(view);
		}

		return view;
	}

	synchronized public View removeView(CharSequence text) {
		View view = findView(text);
		if (view != null) {
			removeView(view);
		}

		return view;
	}

	synchronized public View removeViewAt(int index) {
		View view = getViewAt(index);
		if (view != null) {
			removeView(view);
		}

		return view;
	}

	synchronized public void removeViewAll() {
		if (mViewGroup != null) {
			mViewGroup.removeAllViews();
		}

		mViewMapId.clear();
	}

	// ================================================================================

	synchronized public boolean hasText(CharSequence text) {
		return mViewMapText.containsKey(text);
	}

	synchronized public View addText(CharSequence text, int index) {
		if (hasText(text)) {
			return null;
		}

		View view = createView(text);
		if (view == null || addView(view, index) < 0) {
			return null;
		}

		mViewMapText.put(text, view);

		return view;
	}

	synchronized public void removeText(View view, CharSequence text) {
		removeView(view);
		mViewMapText.remove(text);
	}

	synchronized public View removeText(CharSequence text) {
		View view = mViewMapText.get(text);
		if (view != null) {
			removeText(view, text);
		}

		return view;
	}

	synchronized public void removeText(View view) {
		removeText(view, getViewText(view));
	}

	synchronized public View removeText(int id) {
		View view = findView(id);
		if (view != null) {
			removeText(view);
		}

		return view;
	}

	synchronized public View removeTextAt(int index) {
		View view = getViewAt(index);
		if (view != null) {
			removeText(view);
		}

		return view;
	}

	synchronized public void removeTextAll() {
		removeViewAll();
		mViewMapText.clear();
	}

	// ================================================================================

	public boolean addTopView(View view, LayoutParams params) {
		if (mManager == null) {
			return false;
		}

		mManager.addView(view, params);

		return true;
	}

	protected LayoutParams createRootViewLayoutParams() {
		LayoutParams params = new LayoutParams();

		params.x = params.y = 0;
		params.type = LayoutParams.TYPE_PHONE;
		params.format = PixelFormat.RGBA_8888;
		params.gravity = Gravity.RIGHT | Gravity.TOP;
		params.width = WindowManager.LayoutParams.WRAP_CONTENT;
		params.height = WindowManager.LayoutParams.WRAP_CONTENT;
		params.flags = LayoutParams.FLAG_NOT_FOCUSABLE | LayoutParams.FLAG_NOT_TOUCHABLE;

		return params;
	}

	protected View createRootView() {
		LinearLayout layout = new LinearLayout(getApplicationContext());

		layout.setOrientation(LinearLayout.VERTICAL);

		return layout;
	}

	protected boolean addRootView(View view, LayoutParams params) {
		return addTopView(view, params);
	}

	protected boolean addRootView(View view) {
		return addRootView(view, createRootViewLayoutParams());
	}

	protected View addRootView() {
		View view = createRootView();
		if (view != null && addRootView(view)) {
			return view;
		}

		return null;
	}

	public View findViewById(int id) {
		return mRootView.findViewById(id);
	}

	// ================================================================================

	@Override
	public void onCreate() {
		mManager = (WindowManager) getApplicationContext().getSystemService(WINDOW_SERVICE);

		mRootView = addRootView();
		mViewGroup = findViewGroup();

		doInitialize();

		super.onCreate();
	}

	@Override
	public void onDestroy() {
		removeTextAll();

		if (mRootView != null) {
			mManager.removeView(mRootView);
		}

		mRootView = null;
		mViewGroup = null;

		mViewMapId.clear();
		mViewMapText.clear();

		super.onDestroy();
	}

	@Override
	public IBinder onBind(Intent arg0) {
		return mBinder;
	}
}
