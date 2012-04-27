package org.kde.necessitas.origo;

import android.util.DisplayMetrics;
import android.os.Environment;
import java.util.Locale;

public class JavaFacade
{

	public JavaFacade()
	{

	}

	public static int getDensityDpi()
	{
		DisplayMetrics metrics = new DisplayMetrics();
		QtActivity.sInstance.getWindowManager().getDefaultDisplay().getMetrics(metrics);

		return metrics.densityDpi;
	}

	public static float getDensity()
	{
		DisplayMetrics metrics = new DisplayMetrics();
		QtActivity.sInstance.getWindowManager().getDefaultDisplay().getMetrics(metrics);

		return metrics.density;
	}

	public static String getLocaleString()
	{
		Locale currentLocale = Locale.getDefault();
		return currentLocale.toString();
	}

	public static String getExternalStorageDirectory()
	{
		return Environment.getExternalStorageDirectory().getPath();
	}

	public static boolean isExternalStorageReady()
	{
		return Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState());
	}
}
