using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using InTheHand.Bluetooth;

namespace WT9011BleLib
{
    public class SensorData
    {
        public double AccelX, AccelY, AccelZ;
        public double GyroX, GyroY, GyroZ;
        public double Roll, Pitch, Yaw;

        public override string ToString() =>
            $"Accel: x={AccelX:F2}g y={AccelY:F2}g z={AccelZ:F2}g | " +
            $"Gyro: x={GyroX:F1}°/s y={GyroY:F1}°/s z={GyroZ:F1}°/s | " +
            $"Angle: roll={Roll:F1}° pitch={Pitch:F1}° yaw={Yaw:F1}°";
    }

    public class BleManager
    {
        private const string ServiceUuid = "0000ffe5-0000-1000-8000-00805f9a34fb";
        private const string NotifyCharacteristicUuid = "0000ffe4-0000-1000-8000-00805f9a34fb";

        private BluetoothDevice connectedDevice;
        private GattCharacteristic notifyCharacteristic;

        public event Action<SensorData> OnSensorDataReceived;

        public async Task<List<BluetoothDevice>> ScanDevicesAsync(int timeoutSeconds = 5)
        {
            var devices = new Dictionary<string, BluetoothDevice>();

            void OnAdvertisementReceived(object sender, BluetoothAdvertisingEvent e)
            {
                if (!devices.ContainsKey(e.Device.Id))
                    devices[e.Device.Id] = e.Device;
            }

            Bluetooth.AdvertisementReceived += OnAdvertisementReceived;

            try
            {
                await Bluetooth.RequestLEScanAsync(new BluetoothLEScanOptions { KeepRepeatedDevices = false });
                await Task.Delay(timeoutSeconds * 1000);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ Ошибка сканирования: {ex.Message}");
            }
            finally
            {
                Bluetooth.AdvertisementReceived -= OnAdvertisementReceived;
            }

            return devices.Values.ToList();
        }

        public async Task<bool> ConnectAsync(string deviceId)
        {
            try
            {
                connectedDevice = await BluetoothDevice.FromIdAsync(deviceId);
                if (connectedDevice == null)
                    return false;

                await connectedDevice.Gatt.ConnectAsync();

                var service = await connectedDevice.Gatt.GetPrimaryServiceAsync(Guid.Parse(ServiceUuid));
                if (service == null)
                    return false;

                notifyCharacteristic = await service.GetCharacteristicAsync(Guid.Parse(NotifyCharacteristicUuid));
                if (notifyCharacteristic == null)
                    return false;

                notifyCharacteristic.CharacteristicValueChanged += NotifyCharacteristic_ValueChanged;
                await notifyCharacteristic.StartNotificationsAsync();

                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ Ошибка подключения: {ex.Message}");
                await DisconnectAsync();
                return false;
            }
        }

        public async Task DisconnectAsync()
        {
            try
            {
                if (notifyCharacteristic != null)
                {
                    notifyCharacteristic.CharacteristicValueChanged -= NotifyCharacteristic_ValueChanged;
                    await notifyCharacteristic.StopNotificationsAsync();
                    notifyCharacteristic = null;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"⚠️ Ошибка при отключении от notify: {ex.Message}");
            }

            try
            {
                connectedDevice?.Gatt?.Disconnect();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"⚠️ Ошибка при отключении от устройства: {ex.Message}");
            }

            connectedDevice = null;
        }

        private void NotifyCharacteristic_ValueChanged(object sender, GattCharacteristicValueChangedEventArgs e)
        {
            var data = e.Value;
            var parsed = ParsePacketExtended(data);
            if (parsed != null)
                OnSensorDataReceived?.Invoke(parsed);
        }

        private SensorData ParsePacketExtended(byte[] data)
        {
            if (data.Length < 20 || data[0] != 0x55 || data[1] != 0x61)
                return null;

            try
            {
                short ax = BitConverter.ToInt16(data, 2);
                short ay = BitConverter.ToInt16(data, 4);
                short az = BitConverter.ToInt16(data, 6);

                short gx = BitConverter.ToInt16(data, 8);
                short gy = BitConverter.ToInt16(data, 10);
                short gz = BitConverter.ToInt16(data, 12);

                short roll = BitConverter.ToInt16(data, 14);
                short pitch = BitConverter.ToInt16(data, 16);
                short yaw = BitConverter.ToInt16(data, 18);

                return new SensorData
                {
                    AccelX = ax / 32768.0 * 16,
                    AccelY = ay / 32768.0 * 16,
                    AccelZ = az / 32768.0 * 16,
                    GyroX = gx / 32768.0 * 2000,
                    GyroY = gy / 32768.0 * 2000,
                    GyroZ = gz / 32768.0 * 2000,
                    Roll = roll / 32768.0 * 180,
                    Pitch = pitch / 32768.0 * 180,
                    Yaw = NormalizeYaw(yaw / 32768.0 * 180)
                };
            }
            catch
            {
                return null;
            }
        }

        private double NormalizeYaw(double yaw)
        {
            if (yaw > 180)
                yaw -= 360;
            return yaw;
        }
    }
}
