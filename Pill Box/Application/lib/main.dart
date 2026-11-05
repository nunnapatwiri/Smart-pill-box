import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() => runApp(const SmartPillBoxApp());

class SmartPillBoxApp extends StatelessWidget {
  const SmartPillBoxApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        fontFamily: 'Prompt',
        useMaterial3: true,
        scaffoldBackgroundColor: const Color(0xFFF0F7FF),
      ),
      home: const MainAppScreen(),
    );
  }
}

class MainAppScreen extends StatefulWidget {
  const MainAppScreen({super.key});

  @override
  State<MainAppScreen> createState() => _MainAppScreenState();
}

class _MainAppScreenState extends State<MainAppScreen> {
  bool isConnected = false;
  MqttServerClient? client;
  List<Map<String, dynamic>> medicines = [];
  String lastNotifiedTime = ""; 

  final medNameController = TextEditingController();
  final amountController = TextEditingController(text: "1");
  String selectedHour = "08";
  String selectedMinute = "00";
  String selectedSlot = "ช่อง 1";

  @override
  void initState() {
    super.initState();
    _loadData();
    connectMqtt();
    Timer.periodic(const Duration(seconds: 30), (timer) => checkReminders());
  }

  Future<void> _saveData() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('pill_storage_v2', jsonEncode(medicines));
  }

  Future<void> _loadData() async {
    final prefs = await SharedPreferences.getInstance();
    final String? saved = prefs.getString('pill_storage_v2');
    if (saved != null) {
      setState(() {
        medicines = List<Map<String, dynamic>>.from(jsonDecode(saved));
      });
    }
  }

  Future<void> connectMqtt() async {
    client = MqttServerClient("iot.cpe.ku.ac.th", 'client_pill_${DateTime.now().millisecondsSinceEpoch}');
    client!.port = 1883;
    client!.keepAlivePeriod = 20;
    client!.logging(on: false);

    try {
      client!.connectionMessage = MqttConnectMessage()
          .authenticateAs("b6810503650", "nunnapat.wi@ku.th")
          .withClientIdentifier('client_pill_${DateTime.now().millisecondsSinceEpoch}')
          .startClean();

      await client!.connect();
      setState(() => isConnected = true);
    } catch (e) {
      debugPrint("❌ เชื่อมต่อไม่สำเร็จ: $e");
      setState(() => isConnected = false);
    }
  }

  void checkReminders() {
    if (!isConnected) return;
    final now = DateTime.now();
    final currentTime = "${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}";
    if (currentTime == lastNotifiedTime) return;

    for (var med in medicines) {
      if (med['time'] == currentTime) {
        final builder = MqttClientPayloadBuilder();
        builder.addString(jsonEncode({
          "buzzer": "ON",
          "pillName": med['name'],
          "amount": med['amount'],
          "slot": med['slot'] 
        }));
        client?.publishMessage("b6810503650/pillbox/config", MqttQos.atLeastOnce, builder.payload!);
        lastNotifiedTime = currentTime;
      }
    }
  }

  // ⭐ ฟังก์ชันแสดงหน้าต่างเตือนเมื่อใส่ช่องซ้ำ
  void _showErrorDialog(String message) {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text("⚠️ ไม่สามารถเพิ่มยาได้", style: TextStyle(fontWeight: FontWeight.bold)),
        content: Text(message),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx), child: const Text("ตกลง"))
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("ตั้งค่ารายการยา", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
        backgroundColor: const Color(0xFF3B82F6),
        actions: [
          Padding(
            padding: const EdgeInsets.only(right: 15),
            child: Icon(Icons.circle, size: 12, color: isConnected ? Colors.greenAccent : Colors.redAccent),
          )
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            Container(
              padding: const EdgeInsets.all(25),
              decoration: BoxDecoration(
                color: Colors.white,
                borderRadius: BorderRadius.circular(20),
                boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.05), blurRadius: 15)],
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Row(
                    children: [
                      Icon(Icons.add_circle, color: Color(0xFF3B82F6)),
                      SizedBox(width: 10),
                      Text("เพิ่มรายการยา", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Color(0xFF1E3A8A))),
                    ],
                  ),
                  const SizedBox(height: 20),
                  Row(
                    children: [
                      Expanded(child: _buildTextField("ชื่อยา", "เช่น พาราเซตามอล", medNameController)),
                      const SizedBox(width: 20),
                      Expanded(child: _buildTimePicker()),
                    ],
                  ),
                  const SizedBox(height: 20),
                  Row(
                    children: [
                      Expanded(child: _buildTextField("จำนวน (เม็ด)", "เช่น 1", amountController, isNumber: true)),
                      const SizedBox(width: 20),
                      Expanded(child: _buildDropdown("ช่องยา", ["ช่อง 1", "ช่อง 2", "ช่อง 3"], selectedSlot, (v) => setState(() => selectedSlot = v!))),
                    ],
                  ),
                  const SizedBox(height: 25),
                  ElevatedButton(
                    onPressed: () {
                      if (medNameController.text.isNotEmpty) {
                        // 1. แปลงค่าช่องยา
                        String slotValue = "slot1";
                        if (selectedSlot == "ช่อง 1") slotValue = "slot1";
                        if (selectedSlot == "ช่อง 2") slotValue = "slot2";
                        if (selectedSlot == "ช่อง 3") slotValue = "slot3";

                        // ⭐ 2. เช็กว่าช่องยานี้ถูกใช้ไปหรือยัง (Check Duplicate Slot)
                        bool isDuplicate = medicines.any((m) => m['slot'] == slotValue);

                        if (isDuplicate) {
                          _showErrorDialog("$selectedSlot ถูกใช้งานแล้ว กรุณาลบรายการเดิมออกก่อนพยายามเพิ่มใหม่ครับ");
                        } else {
                          setState(() {
                            medicines.add({
                              'name': medNameController.text,
                              'time': "$selectedHour:$selectedMinute",
                              'amount': amountController.text,
                              'slot': slotValue,
                              'displaySlot': selectedSlot,
                            });
                          });
                          _saveData();
                          medNameController.clear();
                          amountController.text = "1";
                        }
                      } else {
                        _showErrorDialog("กรุณากรอกชื่อยาด้วยครับ");
                      }
                    },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: const Color(0xFF3B82F6),
                      foregroundColor: Colors.white,
                      minimumSize: const Size(double.infinity, 55),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(15)),
                    ),
                    child: const Text("เพิ่มรายการยา", style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 25),
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(20),
              decoration: BoxDecoration(color: Colors.white, borderRadius: BorderRadius.circular(20)),
              child: Column(
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      const Text("📋 รายการยาทั้งหมด", style: TextStyle(fontWeight: FontWeight.bold, color: Color(0xFF1E3A8A))),
                      Container(
                        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                        decoration: BoxDecoration(color: const Color(0xFFE0E7FF), borderRadius: BorderRadius.circular(10)),
                        child: Text("${medicines.length} / 3 ช่อง", style: const TextStyle(fontSize: 12, color: Color(0xFF3B82F6))),
                      )
                    ],
                  ),
                  const SizedBox(height: 15),
                  medicines.isEmpty
                      ? const Text("ยังไม่มีรายการยา", style: TextStyle(color: Colors.grey))
                      : ListView.builder(
                          shrinkWrap: true,
                          physics: const NeverScrollableScrollPhysics(),
                          itemCount: medicines.length,
                          itemBuilder: (context, i) => ListTile(
                            leading: CircleAvatar(
                              backgroundColor: const Color(0xFF3B82F6).withOpacity(0.1),
                              child: const Icon(Icons.medication, color: Color(0xFF3B82F6)),
                            ),
                            title: Text(medicines[i]['name'], style: const TextStyle(fontWeight: FontWeight.bold)),
                            subtitle: Text(
                              "เวลา: ${medicines[i]['time']} น. | ${medicines[i]['amount']} เม็ด | ${medicines[i]['displaySlot'] ?? medicines[i]['slot']}",
                              style: const TextStyle(fontSize: 13),
                            ),
                            trailing: IconButton(
                              icon: const Icon(Icons.delete_outline, color: Colors.redAccent),
                              onPressed: () {
                                setState(() => medicines.removeAt(i));
                                _saveData();
                              },
                            ),
                          ),
                        ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  // --- Widget ส่วนประกอบอื่นคงเดิมตามที่เนสส่งมา ---
  Widget _buildTextField(String label, String hint, TextEditingController ctrl, {bool isNumber = false}) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(label, style: const TextStyle(fontSize: 14, fontWeight: FontWeight.bold)),
        const SizedBox(height: 8),
        TextField(
          controller: ctrl,
          keyboardType: isNumber ? TextInputType.number : TextInputType.text,
          decoration: InputDecoration(
            hintText: hint,
            filled: true,
            fillColor: Colors.blue.withOpacity(0.05),
            border: OutlineInputBorder(borderRadius: BorderRadius.circular(12), borderSide: BorderSide.none),
          ),
        ),
      ],
    );
  }

  Widget _buildDropdown(String label, List<String> items, String current, Function(String?) onChange) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(label, style: const TextStyle(fontSize: 14, fontWeight: FontWeight.bold)),
        const SizedBox(height: 8),
        DropdownButtonFormField<String>(
          value: current,
          decoration: InputDecoration(
            filled: true,
            fillColor: Colors.blue.withOpacity(0.05),
            border: OutlineInputBorder(borderRadius: BorderRadius.circular(12), borderSide: BorderSide.none),
          ),
          items: items.map((e) => DropdownMenuItem(value: e, child: Text(e))).toList(),
          onChanged: onChange,
        ),
      ],
    );
  }

  Widget _buildTimePicker() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text("เวลาทาน", style: TextStyle(fontSize: 14, fontWeight: FontWeight.bold)),
        const SizedBox(height: 8),
        Row(
          children: [
            DropdownButton<String>(
              value: selectedHour,
              items: List.generate(24, (i) => i.toString().padLeft(2, '0')).map((e) => DropdownMenuItem(value: e, child: Text(e))).toList(),
              onChanged: (v) => setState(() => selectedHour = v!),
            ),
            const Text(" : "),
            DropdownButton<String>(
              value: selectedMinute,
              items: List.generate(60, (i) => i.toString().padLeft(2, '0')).map((e) => DropdownMenuItem(value: e, child: Text(e))).toList(),
              onChanged: (v) => setState(() => selectedMinute = v!),
            ),
          ],
        ),
      ],
    );
  }
}