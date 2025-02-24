using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Text;

class Program
{
    private static readonly HashSet<IntPtr> windowHandles = [];
    private static readonly HashSet<IntPtr> changedHandles = []; // 이미 색깔이 변경된 창 핸들러를 추적
    private static readonly CancellationTokenSource cancellationTokenSource = new();
    private static readonly StringBuilder windowTitleBuffer = new(256); // 재사용 가능한 StringBuilder

    static void Main(string[] args)
    {
        if (!IsRunAsAdmin())
        {
            // 관리자 권한으로 재실행
            RestartAsAdmin();
            return;
        }

        Console.WriteLine("Hello, World!");
        StartWindowHandleCollector();
    }

    static bool IsRunAsAdmin()
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            throw new PlatformNotSupportedException("This method is only supported on Windows.");
        }

        using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
        {
            WindowsPrincipal principal = new WindowsPrincipal(identity);
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }
    }

    static void RestartAsAdmin()
    {
        var exeName = Environment.ProcessPath;
        if (exeName == null)
        {
            throw new InvalidOperationException("Unable to determine the executable path.");
        }

        var startInfo = new ProcessStartInfo(exeName)
        {
            UseShellExecute = true,
            Verb = "runas"
        };

        try
        {
            Process.Start(startInfo);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Failed to restart as admin: {ex.Message}");
        }

        Environment.Exit(0);
    }

    static void StartWindowHandleCollector()
    {
        Console.CancelKeyPress += (sender, e) =>
        {
            e.Cancel = true;
            cancellationTokenSource.Cancel();
        };

        var collectorTask = Task.Run(() => RunCollectorAndDisplay(cancellationTokenSource.Token), cancellationTokenSource.Token);

        Console.WriteLine("Press Ctrl+Z to stop...");

        try
        {
            collectorTask.Wait();
        }
        catch (AggregateException ex)
        {
            foreach (var innerEx in ex.InnerExceptions)
            {
                Console.WriteLine(innerEx.Message);
            }
        }
        finally
        {
            cancellationTokenSource.Dispose();
        }
    }

    static async Task RunCollectorAndDisplay(CancellationToken token)
    {
        while (!token.IsCancellationRequested)
        {
            await CollectWindowHandles(token);
            await DisplayWindowHandles(token);
            ChangeWindowBorderColor(RGB(135, 206, 235)); // 하늘색으로 변경
            RemoveClosedHandles(); // 닫힌 창 핸들러를 삭제
        }
    }

    static async Task CollectWindowHandles(CancellationToken token)
    {
        var currentHandles = new List<IntPtr>();
        EnumWindows((hWnd, lParam) =>
        {
            if (IsWindowVisible(hWnd) && !IsToolWindow(hWnd))
            {
                currentHandles.Add(hWnd);
            }
            return true;
        }, IntPtr.Zero);

        lock (windowHandles)
        {
            windowHandles.Clear();
            foreach (var handle in currentHandles)
            {
                windowHandles.Add(handle);
            }
        }

        await Task.Delay(5000, token); // 5초마다 갱신
    }

    static async Task DisplayWindowHandles(CancellationToken token)
    {
        lock (windowHandles)
        {
            Console.Clear();
            Console.WriteLine("\x1b[3J");
            Console.WriteLine("Current Window Handles:");
            foreach (var handle in windowHandles)
            {
                string windowTitle = GetWindowTitle(handle);
                string processName = GetProcessName(handle);
                Console.WriteLine($"Handle: {handle}, Title: {windowTitle}, Process: {processName}");
            }
            Console.WriteLine($"\nTotal Handles: {windowHandles.Count}");
        }
        await Task.Delay(1000, token); // 1초마다 화면 갱신
    }

    static void ChangeWindowBorderColor(uint color)
    {
        foreach (var handle in windowHandles)
        {
            if (!changedHandles.Contains(handle))
            {
                try
                {
                    int colorRef = (int)color;
                    int result = DwmSetWindowAttribute(handle, DWMWA_BORDER_COLOR, ref colorRef, sizeof(int));
                    if (result != 0)
                    {
                        throw new InvalidOperationException($"Failed to set window attribute. Error code: {result}");
                    }
                    changedHandles.Add(handle); // 색깔이 변경된 창 핸들러를 추가
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error changing border color for handle {handle}: {ex.Message}");
                }
            }
        }
    }

    static void RemoveClosedHandles()
    {
        var closedHandles = changedHandles.Where(handle => !IsWindow(handle)).ToList();
        foreach (var handle in closedHandles)
        {
            changedHandles.Remove(handle);
        }
    }

    static string GetWindowTitle(IntPtr hWnd)
    {
        windowTitleBuffer.Clear();
        if (GetWindowText(hWnd, windowTitleBuffer, windowTitleBuffer.Capacity) > 0)
        {
            return windowTitleBuffer.ToString();
        }
        return string.Empty;
    }

    static string GetProcessName(IntPtr hWnd)
    {
        GetWindowThreadProcessId(hWnd, out uint processId);
        try
        {
            Process proc = Process.GetProcessById((int)processId);
            return proc.ProcessName;
        }
        catch
        {
            return "Unknown";
        }
    }

    static bool IsToolWindow(IntPtr hWnd)
    {
        int exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        return (exStyle & WS_EX_TOOLWINDOW) != 0;
    }

    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    private static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern int GetWindowLong(IntPtr hWnd, int nIndex);

    [DllImport("dwmapi.dll", PreserveSig = true)]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int dwAttribute, ref int pvAttribute, int cbAttribute);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsWindowVisible(IntPtr hWnd);

    private const int GWL_EXSTYLE = -20;
    private const int WS_EX_TOOLWINDOW = 0x00000080;
    private const int DWMWA_BORDER_COLOR = 34;

    private static uint RGB(byte r, byte g, byte b)
    {
        return (uint)(r | (g << 8) | (b << 16));
    }
}
