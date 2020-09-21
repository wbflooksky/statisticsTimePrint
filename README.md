# statisticsTimePrint
统一将操作的耗时输出，并提供方便的计算耗时的接口，默认每分钟输出一次结果

{
    TimeConsuming tc("sum")
    Sleep(1000)
}

TimeConsuming tc
tc.addTimeConsumingTag("sub")
Sleep(1000)
tc.drivingEnd("sub")