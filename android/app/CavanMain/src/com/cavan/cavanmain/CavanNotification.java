package com.cavan.cavanmain;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Notification;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.service.notification.StatusBarNotification;

import com.cavan.android.CavanAndroid;
import com.cavan.android.CavanDatabaseProvider.CavanDatabaseTable;

public class CavanNotification {

	public static final String TABLE_NAME = "notification";
	public static final Uri CONTENT_URI = Uri.withAppendedPath(CavanNotificationProvider.CONTENT_URI, TABLE_NAME);

	public static final String KEY_TIMESTAMP = "timestamp";
	public static final String KEY_PACKAGE = "package";
	public static final String KEY_TITLE = "title";
	public static final String KEY_USER_NAME = "user_name";
	public static final String KEY_GROUP_NAME = "group_name";
	public static final String KEY_CONTENT = "content";

	public static final Pattern sGroupPattern = Pattern.compile("([^\\(]+)\\((.+)\\)\\s*$");

	public static final String[] PROJECTION = {
		KEY_TIMESTAMP, KEY_PACKAGE, KEY_TITLE, KEY_USER_NAME, KEY_GROUP_NAME, KEY_CONTENT
	};

	protected long mTimestamp;
	protected String mPackageName;
	protected String mUserName;
	protected String mGroupName;
	protected String mTitle;
	protected String mContent;

	public static void initDatabaseTable(CavanNotificationProvider provider) {
		CavanDatabaseTable table = provider.getTable(TABLE_NAME);

		table.setColumn(KEY_TIMESTAMP, "long");
		table.setColumn(KEY_PACKAGE, "text");
		table.setColumn(KEY_TITLE, "text");
		table.setColumn(KEY_USER_NAME, "text");
		table.setColumn(KEY_GROUP_NAME, "text");
		table.setColumn(KEY_CONTENT, "text");
	}

	public CavanNotification() {
		super();
	}

	public CavanNotification(String packageName, String user, String group, String title, String content) {
		mTimestamp = System.currentTimeMillis();
		mPackageName = packageName;
		mUserName = user;
		mGroupName = group;
		mTitle = title;
		mContent = content;
	}

	public CavanNotification(StatusBarNotification sbn) throws Exception {
		if (!parse(sbn)) {
			throw new Exception("Failed to parse StatusBarNotification");
		}
	}

	public CavanNotification(Cursor cursor) throws Exception {
		if (!parse(cursor)) {
			throw new Exception("Failed to parse Cursor");
		}
	}

	public boolean parse(StatusBarNotification sbn) {
		mTimestamp = System.currentTimeMillis();
		mPackageName = sbn.getPackageName();

		Notification notification = sbn.getNotification();

		CharSequence title = notification.extras.getCharSequence(Notification.EXTRA_TITLE);
		if (title != null) {
			mTitle = title.toString();
		}

		CharSequence text = notification.tickerText;
		if (text == null) {
			text = notification.extras.getCharSequence(Notification.EXTRA_TEXT);
		}

		if (text == null) {
			return false;
		}

		String content = text.toString();

		CavanAndroid.eLog("[" + mTitle + "] ================================================================================");
		CavanAndroid.eLog(content);

		String[] contents = content.split(":", 2);

		if (contents.length < 2) {
			mContent = content.trim();
		} else {
			String name = contents[0].trim();
			Matcher matcher = sGroupPattern.matcher(name);
			if (matcher.find()) {
				mUserName = matcher.group(1);
				mGroupName = matcher.group(2);
			} else {
				mUserName = name;
			}

			mContent = contents[1].trim();
		}

		return true;
	}

	public boolean parse(Cursor cursor) {
		try {
			int column = 0;

			mTimestamp = cursor.getLong(column++);
			mPackageName = cursor.getString(column++);
			mTitle = cursor.getString(column++);
			mUserName = cursor.getString(column++);
			mGroupName = cursor.getString(column++);
			mContent = cursor.getString(column++);

			return true;
		} catch (Exception e) {
			return false;
		}
	}

	// ================================================================================

	public long getTimestamp() {
		return mTimestamp;
	}

	public void setTimestamp(long timestamp) {
		mTimestamp = timestamp;
	}

	public String getPackageName() {
		return mPackageName;
	}

	public void setPackageName(String packageName) {
		mPackageName = packageName;
	}

	public CharSequence getUserName() {
		return mUserName;
	}

	public void setUserName(String name) {
		mUserName = name;
	}

	public CharSequence getGroupName() {
		return mGroupName;
	}

	public void setGroupName(String name) {
		mGroupName = name;
	}

	public CharSequence getTitle() {
		return mTitle;
	}

	public void setTitle(String title) {
		mTitle = title;
	}

	public String getContent() {
		return mContent;
	}

	public void setContent(String content) {
		mContent = content;
	}

	public String buildTitle() {
		DateFormat format = DateFormat.getDateTimeInstance();
		StringBuilder builder = new StringBuilder(format.format(new Date(mTimestamp)));

		if (mUserName != null) {
			builder.append("\n");
			builder.append(mUserName);

			if (mGroupName != null) {
				builder.append("@");
				builder.append(mGroupName);
			}
		} else if (mGroupName != null) {
			builder.append("\n");
			builder.append(mGroupName);
		} else if (mTitle != null) {
			builder.append("\n");
			builder.append(mTitle);
		}

		return builder.toString();
	}

	// ================================================================================

	public ContentValues getContentValues() {
		ContentValues values = new ContentValues(6);

		values.put(KEY_TIMESTAMP, mTimestamp);

		if (mPackageName != null) {
			values.put(KEY_PACKAGE, mPackageName);
		}

		if (mTitle != null) {
			values.put(KEY_TITLE, mTitle.toString());
		}

		if (mUserName != null) {
			values.put(KEY_USER_NAME, mUserName.toString());
		}

		if (mGroupName != null) {
			values.put(KEY_GROUP_NAME, mGroupName.toString());
		}

		if (mContent != null) {
			values.put(KEY_CONTENT, mContent.toString());
		}

		return values;
	}

	public Uri insert(ContentResolver resolver) {
		return resolver.insert(CONTENT_URI, getContentValues());
	}

	public static ArrayList<CavanNotification> parseCursor(Cursor cursor) {
		ArrayList<CavanNotification> notifications = new ArrayList<CavanNotification>();

		if (cursor != null && cursor.moveToFirst()) {
			do {
				try {
					notifications.add(new CavanNotification(cursor));
				} catch (Exception e) {
					break;
				}
			} while (cursor.moveToNext());
		}

		return notifications;
	}

	public static int delete(ContentResolver resolver, String where, String[] selectionArgs) {
		return resolver.delete(CONTENT_URI, where, selectionArgs);
	}

	public static int deleteAll(ContentResolver resolver) {
		return delete(resolver, null, null);
	}

	public static int deleteByPackage(ContentResolver resolver, String pkgName) {
		return delete(resolver, KEY_PACKAGE + "=?", new String[] { pkgName } );
	}

	public static int deleteByUser(ContentResolver resolver, String user) {
		return delete(resolver, KEY_USER_NAME + "=?", new String[] { user } );
	}

	public static int deleteByGroup(ContentResolver resolver, String group) {
		return delete(resolver, KEY_GROUP_NAME + "=?", new String[] { group } );
	}

	public static Cursor query(ContentResolver resolver, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
		return resolver.query(CONTENT_URI, projection, selection, selectionArgs, sortOrder);
	}

	public static Cursor query(ContentResolver resolver, String selection, String[] selectionArgs, String sortOrder) {
		return resolver.query(CONTENT_URI, PROJECTION, selection, selectionArgs, sortOrder);
	}

	public static Cursor queryAll(ContentResolver resolver, String[] projection, String sortOrder) {
		return query(resolver, projection, null, null, sortOrder);
	}

	public static Cursor queryAll(ContentResolver resolver, String sortOrder) {
		return query(resolver, null, null, sortOrder);
	}

	public static ArrayList<CavanNotification> queryNotification(ContentResolver resolver, String selection, String[] selectionArgs, String sortOrder) {
		Cursor cursor = query(resolver, PROJECTION, selection, selectionArgs, sortOrder);
		return parseCursor(cursor);
	}

	public static ArrayList<CavanNotification> queryNotificationAll(ContentResolver resolver, String sortOrder) {
		return queryNotification(resolver, null, null, sortOrder);
	}

	public static ArrayList<CavanNotification> queryNotificationByPackage(ContentResolver resolver, String pkgName, String sortOrder) {
		return queryNotification(resolver, KEY_PACKAGE + "=?", new String[] { pkgName }, sortOrder);
	}

	public static ArrayList<CavanNotification> queryByUser(ContentResolver resolver, String user, String sortOrder) {
		return queryNotification(resolver, KEY_USER_NAME + "=?", new String[] { user }, sortOrder);
	}

	public static ArrayList<CavanNotification> queryNotificationByGroup(ContentResolver resolver, String group, String sortOrder) {
		return queryNotification(resolver, KEY_GROUP_NAME + "=?", new String[] { group }, sortOrder);
	}

	@Override
	public String toString() {
		StringBuilder builder = new StringBuilder();

		builder.append("timestamp = ");
		builder.append(mTimestamp);

		if (mPackageName != null) {
			builder.append(", package = ");
			builder.append(mPackageName);
		}

		if (mTitle != null) {
			builder.append(", title = ");
			builder.append(mTitle);
		}

		if (mUserName != null) {
			builder.append(", user = ");
			builder.append(mUserName);
		}

		if (mGroupName != null) {
			builder.append(", group = ");
			builder.append(mGroupName);
		}

		if (mContent != null) {
			builder.append(", content = ");
			builder.append(mContent);
		}

		return builder.toString();
	}
}
