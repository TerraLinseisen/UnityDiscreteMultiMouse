using UnityEngine;
using System.Runtime.InteropServices;
using System.Collections.Generic;

public class DiscreteMultiMouse : MonoBehaviour {
    struct ButtonState {
        [MarshalAs(UnmanagedType.U1)]
        public bool down, up, held;
    }
    struct MouseState {
        public int x, y;
        public ButtonState left, right;
    }

    static MouseState[] mouseStates;
    static List<float> sensitivities;

    [DllImport("DiscreteMultiMouse")]
    static extern void unityplugin_init();
    [DllImport("DiscreteMultiMouse")]
    static extern void unityplugin_poll(
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.Struct, SizeParamIndex = 1)]
        out MouseState[] arr, out int len
    );
    [DllImport("DiscreteMultiMouse")]
    static extern void unityplugin_resetMouseStates();
    [DllImport("DiscreteMultiMouse")]
    static extern void unityplugin_resetMouseList();
    [DllImport("DiscreteMultiMouse")]
    static extern void unityplugin_resetDeviceHwnds();
    [DllImport("DiscreteMultiMouse")]
    static extern void unityplugin_kill();

    public static int MouseCount() {
        return mouseStates.Length;
    }

    public static Vector2 Delta(int mouse) {
        if (mouse >= mouseStates.Length || mouse < 0) {
            return Vector2.zero;
        }
        return new Vector2(mouseStates[mouse].x, mouseStates[mouse].y) * sensitivities[mouse];
    }

    public static bool GetButton(int mouse, int button) {
        if (mouse >= mouseStates.Length || mouse < 0) {
            return false;
        }
        switch (button) {
            case 0:
                return mouseStates[mouse].left.held;
            case 1:
                return mouseStates[mouse].right.held;
            default:
                return false;
        }
    }

    public static bool GetButtonDown(int mouse, int button) {
        if (mouse >= mouseStates.Length || mouse < 0) {
            return false;
        }
        switch (button) {
            case 0:
                return mouseStates[mouse].left.down;
            case 1:
                return mouseStates[mouse].right.down;
            default:
                return false;
        }
    }

    public static bool GetButtonUp(int mouse, int button) {
        if (mouse >= mouseStates.Length || mouse < 0) {
            return false;
        }
        switch (button) {
            case 0:
                return mouseStates[mouse].left.up;
            case 1:
                return mouseStates[mouse].right.up;
            default:
                return false;
        }
    }

    public static float GetSensitivity(int mouse) {
        if (mouse >= sensitivities.Count || mouse < 0) {
            return 1.0f;
        }
        return sensitivities[mouse];
    }

    public static void SetSensitivity(int mouse, float sensitivity) {
        if (mouse >= sensitivities.Count || mouse < 0) {
            return;
        }
        sensitivities[mouse] = sensitivity;
    }

    static DiscreteMultiMouse() {
        mouseStates = new MouseState[0];
        sensitivities = new List<float>();
        unityplugin_resetMouseList();
        unityplugin_init();
    }

    void Awake() {
        Cursor.visible = false;
        Cursor.lockState = CursorLockMode.Locked;
    }

    // wrapper function because invoke won't accept a static function directly
    void ResetDeviceHwnds() {
        unityplugin_resetDeviceHwnds();
    }
    void OnApplicationFocus(bool hasFocus) {
        if (hasFocus) {
            // Unity sets the HWNDs associated with RawInputDevices one frame after
            // OnApplicationFocus is called.
            //
            // I need to undo that to recieve input, and Invoke with a time of 0 delays
            // my function just long enough.
            Invoke("ResetDeviceHwnds", 0.0f);
        }
    }

    void OnApplicationQuit() {
        unityplugin_kill();
    }

    void Update() {
        unityplugin_poll(out mouseStates, out int length);
        unityplugin_resetMouseStates();
        while (length > sensitivities.Count) {
            sensitivities.Add(1.0f);
        }
    }
}
