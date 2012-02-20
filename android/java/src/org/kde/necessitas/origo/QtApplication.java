/*
    Copyright (c) 2009-2011, BogDan Vatra <bog_dan_ro@yahoo.com>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the  BogDan Vatra <bog_dan_ro@yahoo.com> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY BogDan Vatra <bog_dan_ro@yahoo.com> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL BogDan Vatra <bog_dan_ro@yahoo.com> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.kde.necessitas.origo;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;

import android.app.Application;

public class QtApplication extends Application
{
    public final static String QtTAG="Qt";
    public static Object m_delegateObject = null;
    public static HashMap<String, ArrayList<Method>> m_delegateMethods= new HashMap<String, ArrayList<Method>>();
    public static Method dispatchKeyEvent = null;
    public static Method dispatchPopulateAccessibilityEvent = null;
    public static Method dispatchTouchEvent = null;
    public static Method dispatchTrackballEvent = null;
    public static Method onKeyDown = null;
    public static Method onKeyMultiple = null;
    public static Method onKeyUp = null;
    public static Method onTouchEvent = null;
    public static Method onTrackballEvent = null;
    public static Method onActivityResult = null;
    public static Method onCreate = null;
    public static Method onKeyLongPress = null;
    public static Method dispatchKeyShortcutEvent = null;
    public static Method onKeyShortcut = null;
    public static Method dispatchGenericMotionEvent = null;
    public static Method onGenericMotionEvent = null;
/*
    public static Method onTerminate = null;
    public static Method onApplyThemeResource = null;
    public static Method onChildTitleChanged = null;
    public static Method onConfigurationChanged = null;
    public static Method onContentChanged  = null;
    public static Method onContextItemSelected  = null;
    public static Method onContextMenuClosed = null;
    public static Method onCreateContextMenu = null;
    public static Method onCreateDescription = null;
    public static Method onCreateDialog = null;
    public static Method onCreateOptionsMenu = null;
    public static Method onCreatePanelMenu = null;
    public static Method onCreatePanelView = null;
    public static Method onCreateThumbnail = null;
    public static Method onCreateView = null;
    public static Method onDestroy = null;
    public static Method onLowMemory = null;
    public static Method onMenuItemSelected = null;
    public static Method onMenuOpened = null;
    public static Method onNewIntent = null;
    public static Method onOptionsItemSelected = null;
    public static Method onOptionsMenuClosed = null;
    public static Method onPanelClosed = null;
    public static Method onPause = null;
    public static Method onPostCreate = null;
    public static Method onPostResume = null;
    public static Method onPrepareDialog = null;
    public static Method onPrepareOptionsMenu = null;
    public static Method onPreparePanel = null;
    public static Method onRestart = null;
    public static Method onRestoreInstanceState = null;
    public static Method onResume = null;
    public static Method onRetainNonConfigurationInstance = null;
    public static Method onSaveInstanceState = null;
    public static Method onSearchRequested = null;
    public static Method onStart = null;
    public static Method onStop = null;
    public static Method onTitleChanged = null;
    public static Method onUserInteraction = null;
    public static Method onUserLeaveHint = null;
    public static Method onWindowAttributesChanged = null;
    public static Method onWindowFocusChanged = null;
    public static Method onAttachedToWindow = null;
    public static Method onBackPressed = null;
    public static Method onDetachedFromWindow = null;
    public static Method onCreateDialog8 = null;
    public static Method onPrepareDialog8 = null;
    public static Method onActionModeFinished = null;
    public static Method onActionModeStarted = null;
    public static Method onAttachFragment = null;
    public static Method onCreateView11 = null;
    public static Method onWindowStartingActionMode = null;
*/

    public static void setQtActivityDelegate(Object listener)
    {
        QtApplication.m_delegateObject = listener;

        ArrayList<Method> delegateMethods = new ArrayList<Method>();
        for (Method m: listener.getClass().getMethods())
            if (m.getDeclaringClass().getName().startsWith("org.kde.necessitas"))
                delegateMethods.add(m);

        ArrayList<Field> applicationFields = new ArrayList<Field>();
        for (Field f: QtApplication.class.getFields())
            if (f.getDeclaringClass().getName().equals(QtApplication.class.getName()))
                applicationFields.add(f);

        for (Method delegateMethod:delegateMethods)
        {
            try {
                QtActivity.class.getDeclaredMethod(delegateMethod.getName(), delegateMethod.getParameterTypes());
                if (QtApplication.m_delegateMethods.containsKey(delegateMethod.getName()))
                    QtApplication.m_delegateMethods.get(delegateMethod.getName()).add(delegateMethod);
                else
                {
                    ArrayList<Method> delegateSet = new ArrayList<Method>();
                    delegateSet.add(delegateMethod);
                    QtApplication.m_delegateMethods.put(delegateMethod.getName(), delegateSet);
                }
                for(Field applicationField:applicationFields)
                {
                    if (applicationField.getName().equals(delegateMethod.getName()))
                    {
                        try {
                            applicationField.set(null, delegateMethod);
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
            catch (Exception e)
            {
            }
        }
    }

    @Override
    public void onTerminate() {
        if (m_delegateObject != null && m_delegateMethods.containsKey("onTerminate"))
            invokeDelegateMethod(m_delegateMethods.get("onTerminate").get(0));
        super.onTerminate();
    }

    public static class InvokeResult
    {
        public boolean invoked = false;
        public Object methodReturns = null;
    }

    private static int stackDeep=-1;
    public static InvokeResult invokeDelegate(Object... args)
    {
        InvokeResult result = new InvokeResult();
        if (m_delegateObject==null)
            return result;
        StackTraceElement[] elements=Thread.currentThread().getStackTrace();
        if (-1 == stackDeep)
        {
            String activityClassName=QtActivity.class.getCanonicalName();
            for(int it=0;it<elements.length;it++)
                if (elements[it].getClassName().equals(activityClassName))
                {
                    stackDeep=it;
                    break;
                }
        }
        final String methodName=elements[stackDeep].getMethodName();
        if (-1 == stackDeep || !m_delegateMethods.containsKey(methodName))
            return result;

        for (Method m:m_delegateMethods.get(methodName))
            if (m.getParameterTypes().length == args.length)
            {
                result.methodReturns=invokeDelegateMethod(m, args);
                result.invoked=true;
                return result;
            }
        return result;
    }

    public static Object invokeDelegateMethod(Method m, Object... args)
    {
        try {
            return m.invoke(m_delegateObject, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
