
var ws = null;
const wsURL = "ws://localhost:8080";

function InitializeWebSocket()
{
    ws = new WebSocket(wsURL);
    ws.binaryType = "arraybuffer";
    ws.onopen = function (ev)
    {
        console.log(`Web Socket open on ${wsURL}`);
        $("#wsState").text("Open");
    };
    ws.onclose = function (ev)
    {
        console.log(`Web Socket closed on ${wsURL}`);
        $("#wsState").text("Closed");
    };
    ws.onmessage = function (ev)
    {
        const obj = JSON.parse(ev.data);
        console.log(obj);
    };
    ws.onerror = function (ev)
    {
        $("#wsState").text("Error - see console log");
        console.log(ev);
    };
}

$(document).ready(
    function ()
    {
        InitializeWebSocket();
    }
);



$("#sliderControl").on("input",
    function ()
    {
        var val = $("#sliderControl").val();
        $("#sliderText").text(val);

        var dataObj = { "type": "sliderMoved", "value": val };
        ws.send(JSON.stringify(dataObj));
    }
);

$("#loadButton").click(function () {
    $("#div1").load("page2.html");
});