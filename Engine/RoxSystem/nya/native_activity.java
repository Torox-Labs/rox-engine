//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

package nya;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.content.res.AssetManager;
import android.text.InputType;
import android.view.*;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.RelativeLayout;

public class native_activity extends Activity implements SurfaceHolder.Callback, View.OnTouchListener
{
    protected static ViewGroup m_layout;
    protected static SurfaceView m_view;
    protected static AssetManager m_asset_mgr;
    private static boolean m_library_loaded;
    private static native_activity m_activity;
    private static boolean m_keyboard_visible;

    protected void onLoadNativeLibrary() //override to load your own
    {
        System.loadLibrary("nya_native");
    }

    protected void onSpawnMain() //override to spawn manually
    {
        native_spawn_main();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        if(!m_library_loaded)
        {
            onLoadNativeLibrary();
            m_library_loaded=true;
        }

        super.onCreate(savedInstanceState);
        m_activity=this;

        m_asset_mgr=getAssets();
        native_set_asset_mgr(m_asset_mgr);
        native_set_user_path(getFilesDir().getPath()+"/");

        m_view=new SurfaceView(getApplication())
        {
            @Override
            public InputConnection onCreateInputConnection(EditorInfo outAttrs)
            {
                return nya_onCreateInputConnection(outAttrs);
            }
        };
        m_view.getHolder().addCallback(this);
        m_view.setOnTouchListener(this);

        m_layout=new RelativeLayout(this);
        m_layout.addView(m_view);
        setContentView(m_layout);

        onSpawnMain();
    }

    protected int requestedInputType=InputType.TYPE_CLASS_TEXT;
    protected int requestedImeAction=EditorInfo.IME_ACTION_NONE;
    protected InputConnection nya_onCreateInputConnection(EditorInfo outAttrs)
    {
        outAttrs.inputType=requestedInputType;
        outAttrs.imeOptions=requestedImeAction;
        return null;
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        native_resume();
    }

    @Override
    protected void onPause()
    {
        if(m_keyboard_visible)
            setVirtualKeyboard('h');
        super.onPause();
        native_pause();
    }

    @Override
    protected void onDestroy()
    {
        native_exit();
        super.onDestroy();
        m_layout=null;
        m_view=null;
    }

    @Override
    public boolean onTouch(View v, MotionEvent event)
    {
        switch(event.getActionMasked())
        {
            case MotionEvent.ACTION_MOVE:
                for(int i = 0;i<event.getPointerCount();++i)
                    native_touch((int)event.getX(i),(int)event.getY(i),event.getPointerId(i),true,false);
                break;

            case MotionEvent.ACTION_UP:
                native_touch((int)event.getX(0),(int)event.getY(0),event.getPointerId(0),false,true);
                break;
            case MotionEvent.ACTION_DOWN:
                native_touch((int)event.getX(0),(int)event.getY(0),event.getPointerId(0),true,true);
                break;

            case MotionEvent.ACTION_POINTER_UP:
                int i=event.getActionIndex();
                native_touch((int)event.getX(i),(int)event.getY(i),event.getPointerId(i),false,true);
                break;
            case MotionEvent.ACTION_POINTER_DOWN:
                i=event.getActionIndex();
                native_touch((int)event.getX(i),(int)event.getY(i),event.getPointerId(i),true,true);
                break;

            default:
                break;
        }

        return true;
    }

    @Override
    public boolean onKeyMultiple(int keyCode,int count,KeyEvent event)
    {
        if(keyCode!=KeyEvent.KEYCODE_UNKNOWN)
            return false;

        CharSequence chs=event.getCharacters();
        int chsl=chs.length();
        for(int i=0;i<event.getCharacters().length();++i)
        {
            int ch=event.getCharacters().charAt(i);
            native_key(0,true,ch,event.getRepeatCount()>0);
            native_key(0,false,ch,event.getRepeatCount()>0);
        }
        return true;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        return native_key(keyCode,true,event.getUnicodeChar(),event.getRepeatCount()>0);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        return native_key(keyCode,false,event.getUnicodeChar(),false);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
    {
        native_set_surface(holder.getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {}

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        native_set_surface(null);
    }

    public static void setVirtualKeyboard(final int type)
    {
        m_activity.runOnUiThread(new Runnable() { public void run()
        {
            boolean show=true;
            switch (type)
            {
                case 'n': m_activity.requestedInputType=InputType.TYPE_CLASS_NUMBER; break;
                case 'd': m_activity.requestedInputType=InputType.TYPE_CLASS_NUMBER|InputType.TYPE_NUMBER_FLAG_SIGNED|InputType.TYPE_NUMBER_FLAG_DECIMAL; break;
                case 'f': m_activity.requestedInputType=InputType.TYPE_CLASS_PHONE; break;
                case 't': m_activity.requestedInputType=InputType.TYPE_NULL; break;
                case 'p': m_activity.requestedInputType=InputType.TYPE_NUMBER_VARIATION_PASSWORD; break;
                case 'e': m_activity.requestedInputType=InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS; break;
                case 'w': m_activity.requestedInputType=InputType.TYPE_TEXT_VARIATION_PASSWORD; break;
                case 'u': m_activity.requestedInputType=InputType.TYPE_TEXT_VARIATION_URI; break;
                default: show=false; break;
            }
            m_keyboard_visible=show;
            m_view.setFocusable(show);
            m_view.setFocusableInTouchMode(show);
            InputMethodManager imm=(InputMethodManager)m_activity.getSystemService(Context.INPUT_METHOD_SERVICE);
            if(show)
            {
                m_view.requestFocus();
                imm.showSoftInput(m_view,InputMethodManager.SHOW_FORCED);
                imm.restartInput(m_view);
            }
            else
            {
                m_view.clearFocus();
                imm.hideSoftInputFromWindow(m_view.getWindowToken(),0);
            }
        }});
    }

    public static native void native_spawn_main();
    public static native void native_resume();
    public static native void native_pause();
    public static native void native_exit();
    public static native void native_touch(int x,int y,int id,boolean pressed,boolean btn);
    public static native boolean native_key(int code,boolean pressed,int unicode_char,boolean autorepeat);
    public static native void native_set_surface(Surface surface);
    public static native void native_set_asset_mgr(AssetManager mgr);
    public static native void native_set_user_path(String path);
}
