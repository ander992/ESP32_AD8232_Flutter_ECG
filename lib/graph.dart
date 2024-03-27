import 'package:flutter/material.dart';
import 'package:syncfusion_flutter_charts/charts.dart';

class RealTimeGraph extends StatefulWidget {
  final List<int> data;

  const RealTimeGraph({super.key, required this.data});

  @override
  State<RealTimeGraph> createState() => _RealTimeGraphState();
}

class _RealTimeGraphState extends State<RealTimeGraph> {
  late List<ChartData> chartData;

  @override
  void initState() {
    chartData = _generateChartData();
    super.initState();
  }

  @override
  void didUpdateWidget(covariant RealTimeGraph oldWidget) {
    super.didUpdateWidget(oldWidget);
    setState(() {
      chartData = _generateChartData();
    });
  }

  List<ChartData> _generateChartData() {
    int currentIndex = 0;
    return widget.data.map((value) {
      final chartData = ChartData(currentIndex.toString(), value);
      currentIndex++;
      return chartData;
    }).toList();
  }

  @override
  Widget build(BuildContext context) {
    return SfCartesianChart(
      primaryXAxis: CategoryAxis(),
      series: <ChartSeries>[
        LineSeries<ChartData, String>(
          dataSource: chartData,
          xValueMapper: (ChartData data, _) => data.time,
          yValueMapper: (ChartData data, _) => data.value,
        ),
      ],
    );
  }
}

class ChartData {
  final String time;
  final int value;

  ChartData(
    this.time,
    this.value,
  );
}
