

const slider = document.querySelector('#sliderControl');
const sliderText = document.querySelector('#sliderText');
const state = document.querySelector('#wsState');

var ws = new WebSocket("ws://localhost:8080");
ws.binaryType = "arraybuffer";
ws.onopen = function (ev) {
    state.innerHTML = "Open";
};
ws.onclose = function (ev) {
    state.innerHTML = "Closed";
};
ws.onmessage = function (ev) {

    const obj = JSON.parse(ev.data);
    console.log(ev);
};
ws.onerror = function (ev) {
    state.innerHTML = "Error - see console log";
    console.log(ev);
};


sliderText.innerHTML = slider.value;
slider.oninput = function () {
    sliderText.innerHTML = slider.value;

    var dataObj = { "type": "sliderMoved", "value": slider.value };
    ws.send(JSON.stringify(dataObj));
}