using System;
using System.Threading.Tasks;
using WT9011BleLib;

class Program
{
    static async Task Main()
    {
        var manager = new BleManager();

        manager.OnSensorDataReceived += data =>
        {
            Console.Clear();
            Console.WriteLine($"[Sensor] {data}");
            Console.WriteLine("\nНажмите Enter для выхода...");
        };

        try
        {
            Console.WriteLine("Сканируем BLE устройства...");
            var devices = await manager.ScanDevicesAsync();

            if (devices.Count == 0)
            {
                Console.WriteLine("Устройства не найдены");
                return;
            }

            for (int i = 0; i < devices.Count; i++)
                Console.WriteLine($"{i}: {devices[i].Name ?? "Unnamed"} - {devices[i].Id}");

            Console.Write("Введите номер устройства для подключения: ");
            var input = Console.ReadLine();
            if (!int.TryParse(input, out int idx) || idx < 0 || idx >= devices.Count)
            {
                Console.WriteLine("Неверный индекс.");
                return;
            }

            Console.WriteLine("Подключаемся...");
            var success = await manager.ConnectAsync(devices[idx].Id);
            if (!success)
            {
                Console.WriteLine("Не удалось подключиться.");
                return;
            }

            Console.Clear();
            Console.WriteLine("Подключено! Получаем данные...\n");
            Console.WriteLine("Нажмите Enter для выхода.");
            await Task.Run(() => Console.ReadLine());
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Ошибка: {ex.Message}");
        }
        finally
        {
            await manager.DisconnectAsync();
            Console.WriteLine("Отключено.");
        }
    }
}