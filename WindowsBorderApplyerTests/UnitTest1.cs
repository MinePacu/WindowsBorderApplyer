using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

using Xunit;

public class WindowBorderApplyerTests
{
    [Fact]
    public void TestChangeWindowBorderColor()
    {
        // Arrange
        var windowHandles = new HashSet<IntPtr> { IntPtr.Zero }; // 예제 핸들러 추가
        uint color = RGB(135, 206, 235); // 하늘색

        // Act
        bool result = ChangeWindowBorderColor(windowHandles, color);

        // Assert
        Assert.True(result);
    }

    private static bool ChangeWindowBorderColor(HashSet<IntPtr> windowHandles, uint color)
    {
        bool success = true;
        foreach (var handle in windowHandles)
        {
            int colorRef = (int)color;
            int hr = DwmSetWindowAttribute(handle, DWMWA_BORDER_COLOR, ref colorRef, sizeof(int));
            if (hr != S_OK)
            {
                success = false;
            }
        }
        return success;
    }

    private static uint RGB(byte r, byte g, byte b)
    {
        return (uint)(r | (g << 8) | (b << 16));
    }

    [DllImport("dwmapi.dll", PreserveSig = true)]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int dwAttribute, ref int pvAttribute, int cbAttribute);

    private const int DWMWA_BORDER_COLOR = 34;
    private const int S_OK = 0;
}

