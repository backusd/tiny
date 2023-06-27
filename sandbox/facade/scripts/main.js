//const slider = document.querySelector('#sliderControl');
//const sliderText = document.querySelector('#sliderText');
//const state = document.querySelector('#wsState');
//
//var ws = new WebSocket("ws://localhost:8080");
//ws.binaryType = "arraybuffer";
//ws.onopen = function (ev) {
//    state.innerHTML = "Open";
//};
//ws.onclose = function (ev) {
//    state.innerHTML = "Closed";
//};
//ws.onmessage = function (ev) {
//
//    const obj = JSON.parse(ev.data);
//    console.log(ev);
//};
//ws.onerror = function (ev) {
//    state.innerHTML = "Error - see console log";
//    console.log(ev);
//};
//
//
//sliderText.innerHTML = slider.value;
//slider.oninput = function () {
//    sliderText.innerHTML = slider.value;
//
//    var dataObj = { "type": "sliderMoved", "value": slider.value };
//    ws.send(JSON.stringify(dataObj));
//}

var ws = null;
$("#wsConnect").click(function () {
    console.log("Connecting to websocket...");
    ws = new WebSocket("ws://localhost:8080");
    ws.binaryType = "arraybuffer";
    ws.onopen = function (ev) {
        console.log("WS Open");
        $("#wsState").text("Open");

    };
    ws.onclose = function (ev) {
        console.log("WS Closed");
        $("#wsState").text("Closed");
    };
    ws.onmessage = function (ev) {

        const obj = JSON.parse(ev.data);
        console.log(obj);
    };
    ws.onerror = function (ev) {
        $("#wsState").text("Error - see console log");
        console.log(ev);
    };
});

$("#sliderControl").on("input", function () {

    var val = $("#sliderControl").val();
    $("#sliderText").text(val);

    var dataObj = { "type": "sliderMoved", "value": val };
    ws.send(JSON.stringify(dataObj));
});

//const slider = document.querySelector('#sliderControl');
//const sliderText = document.querySelector('#sliderText');
//slider.oninput = function () {
//    sliderText.innerHTML = slider.value;
//
//    var dataObj = { "type": "sliderMoved", "value": slider.value };
//    ws.send(JSON.stringify(dataObj));
//}

$("#loadButton").click(function () {
    $("#div1").load("page2.html");
});