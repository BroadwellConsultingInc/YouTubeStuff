// See https://aka.ms/new-console-template for more information


using System.Diagnostics;
using SerialWombat;

SerialWombatChip serialWombatChip = new SerialWombatChip();

serialWombatChip.begin("COM98");

Console.WriteLine($"Serial Wombat Firmware Version {serialWombatChip.readVersion()}");
SerialWombatPWM[] meters = new SerialWombatPWM[8];


for (int i = 0; i < 8; i++)
{
    meters[i] = new SerialWombatPWM(serialWombatChip);
    meters[i].begin((byte)(12 + i));
}


PerformanceCounter[] performanceCounters = new PerformanceCounter[32];

for (int i = 0; i < 32; ++i)
{
    performanceCounters[i] = new PerformanceCounter("Processor", "% Processor Time", $"{i}");
    var firstCall = performanceCounters[i].NextValue();
}
Thread.Sleep(1000);

while (true)
{
    Thread.Sleep(100);
    for (int x = 0; x < 32; x+=4)
    {
        float Sum = performanceCounters[x].NextValue();
        Sum += performanceCounters[x+1].NextValue();
        Sum += performanceCounters[x + 2].NextValue();
        Sum += performanceCounters[x + 3].NextValue();
        Sum /= 4;
        UInt16 pwm = (UInt16)(Sum * 65535 / 100);
        Console.WriteLine($"{pwm} {Sum}" );
        meters[x / 4].writeDutyCycle(pwm);
    }
    Console.WriteLine(); Console.WriteLine();

}

