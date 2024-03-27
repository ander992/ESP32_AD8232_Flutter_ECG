import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:firebase_core/firebase_core.dart';

import 'firebase_options.dart';

import 'firebase_wifi.dart';

/*
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>


#define WIFI_SSID "5G proba"
#define WIFI_PASSWORD "opetkrompir"
#define FIREBASE_API_KEY "AIzaSyDn4DhfiPbKhlihaaVjxbKM-VJ-lhrmy0A"
#define FIREBASE_HOST "https://aes-projekat-default-rtdb.europe-west1.firebasedatabase.app/"
*/

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );
  SystemChrome.setPreferredOrientations([DeviceOrientation.landscapeLeft])
      .then((_) {
    runApp(const MyApp());
  });
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      home: FirebaseWiFi(),
    );
  }
}
