import 'dart:math';
import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';

class FirebaseWiFi extends StatefulWidget {
  const FirebaseWiFi({super.key});

  @override
  State<FirebaseWiFi> createState() => _FirebaseWiFiState();
}

class _FirebaseWiFiState extends State<FirebaseWiFi> {
  final database = FirebaseDatabase.instance.ref();
  final List<double> sensorData = [];

  @override
  void initState() {
    super.initState();
    _startDataAcquisition();
  }

  @override
  void dispose() {
    super.dispose();
  }

  void _startDataAcquisition() {
    database.child('test/RRinterval').onValue.listen((event) {
      final Object? data = event.snapshot.value;
      setState(() {
        sensorData.add(double.parse(data.toString()));
      });
    });
  }

  double calculateMEANRR(List<double> data) {
    double sum = 0;
    for (var e in data) {
      sum += e;
    }
    return sum / data.length;
  }

  double calculateRMSSD(List<double> data) {
    double result = 0;
    double sum = 0;
    for (var e in data) {
      sum += pow((e - calculateMEANRR(data)), 2);
    }
    result = sqrt(sum / data.length);
    return result;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('AES'),
        centerTitle: true,
      ),
      body: Column(
        children: [
          Expanded(
            flex: 1,
            child: Center(
              child: Text(
                'Current RR Interval: ${sensorData.last.toStringAsFixed(2)} ms',
                style: const TextStyle(
                  fontSize: 60,
                ),
              ),
            ),
          ),
          Expanded(
            flex: 1,
            child: Center(
              child: Text(
                'BPM: ${(60000.0 / calculateMEANRR(sensorData)).truncate()}',
                style: const TextStyle(
                  fontSize: 60,
                ),
              ),
            ),
          ),
          Expanded(
            flex: 1,
            child: Center(
              child: Text(
                'MEANRR: ${calculateMEANRR(sensorData).toStringAsFixed(2)} ms',
                style: const TextStyle(
                  fontSize: 60,
                ),
              ),
            ),
          ),
          Expanded(
            flex: 1,
            child: Center(
              child: Text(
                'RMSSD: ${calculateRMSSD(sensorData).toStringAsFixed(2)} ms',
                style: const TextStyle(
                  fontSize: 60,
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
