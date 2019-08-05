using UnityEngine;
using System.Runtime.InteropServices;

public class DiscreteMultiMouse : MonoBehaviour {
    private struct ButtonState {
        [MarshalAs(UnmanagedType.U1)]
        public bool down, up, held;
    }
    private struct MouseState {
        public int x, y;
        public ButtonState left, right;
    }

    private static MouseState[] mouseStates = new MouseState[0];

    [DllImport("DiscreteMultiMouse")]
    private static extern void unityplugin_init();
    [DllImport("DiscreteMultiMouse")]
    private static extern void unityplugin_poll(
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.Struct, SizeParamIndex = 1)]
        out MouseState[] arr, out int len
    );
    [DllImport("DiscreteMultiMouse")]
    private static extern void unityplugin_reset();
    [DllImport("DiscreteMultiMouse")]
    private static extern void unityplugin_kill();

    public static int MouseCount() {
        return mouseStates.Length;
    }

    public static Vector2 MouseDelta(uint mouse) {
        if (mouse >= mouseStates.Length) {
            return Vector2.zero;
        }
        return new Vector2(mouseStates[mouse].x, mouseStates[mouse].y);
    }

    public static bool GetButton(uint mouse, uint button) {
        if (mouse >= mouseStates.Length) {
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

    public static bool GetButtonDown(uint mouse, uint button) {
        if (mouse >= mouseStates.Length) {
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

    public static bool GetButtonUp(uint mouse, uint button) {
        if (mouse >= mouseStates.Length) {
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

    void OnApplicationFocus(bool hasFocus) {
        if (hasFocus) {
            unityplugin_init();
            Debug.Log("MyInput.unityplugin_init()");
            Cursor.lockState = CursorLockMode.Locked;
        } else {
            unityplugin_kill();
            Debug.Log("MyInput.unityplugin_kill()");
            Cursor.lockState = CursorLockMode.None;
        }
    }

    void Update() {
        unityplugin_poll(out mouseStates, out int length);
        unityplugin_reset();
    }
}
