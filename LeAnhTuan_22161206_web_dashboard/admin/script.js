const allSideMenu = document.querySelectorAll('#sidebar .side-menu.top li a');

allSideMenu.forEach(item => {
    const li = item.parentElement;

    item.addEventListener('click', function () {
        allSideMenu.forEach(i => {
            i.parentElement.classList.remove('active');
        });
        li.classList.add('active');
        
        localStorage.setItem("activeMenu", this.getAttribute("href"));
    });
});

window.addEventListener("DOMContentLoaded", () => {
    const activeHref = localStorage.getItem("activeMenu");
    if (activeHref) {
        allSideMenu.forEach(item => {
            const li = item.parentElement;
            if (item.getAttribute("href") === activeHref) {
                li.classList.add("active");
            } else {
                li.classList.remove("active");
            }
        });
    }
});

const menuBar = document.querySelector('#content nav .bx.bx-menu');
const sidebar = document.getElementById('sidebar');

menuBar.addEventListener('click', function () {
    sidebar.classList.toggle('hide');
});

let lastTime = "";
function updateRealTimeClock() {
    let time = new Date();
    let gio = String(time.getHours()).padStart(2, '0');
    let phut = String(time.getMinutes()).padStart(2, '0');
    let giay = String(time.getSeconds()).padStart(2, '0');
    let currentTime = `${gio}:${phut}:${giay}`;
    if (lastTime !== currentTime) {
        document.getElementById("real-time-clock").innerText = currentTime;
        lastTime = currentTime;
    }
}

updateRealTimeClock();
setInterval(updateRealTimeClock, 1000);

const searchButton = document.querySelector('#content nav form .form-input button');
const searchButtonIcon = document.querySelector('#content nav form .form-input button .bx');
const searchForm = document.querySelector('#content nav form');

if (searchButton) {
    searchButton.addEventListener('click', function (e) {
        if (window.innerWidth < 576) {
            e.preventDefault();
            searchForm.classList.toggle('show');
            if (searchForm.classList.contains('show')) {
                searchButtonIcon.classList.replace('bx-search', 'bx-x');
            } else {
                searchButtonIcon.classList.replace('bx-x', 'bx-search');
            }
        }
    });
}

if (window.innerWidth < 768) {
    sidebar.classList.add('hide');
} else if (window.innerWidth > 576) {
    if (searchButtonIcon) {
        searchButtonIcon.classList.replace('bx-x', 'bx-search');
    }
    if (searchForm) {
        searchForm.classList.remove('show');
    }
}

window.addEventListener('resize', function () {
    if (this.innerWidth > 576) {
        if (searchButtonIcon) {
            searchButtonIcon.classList.replace('bx-x', 'bx-search');
        }
        if (searchForm) {
            searchForm.classList.remove('show');
        }
    }
});

const switchMode = document.getElementById('switch-mode');

window.addEventListener('load', () => {
    const isDarkMode = localStorage.getItem('darkMode') === 'true';
    switchMode.checked = isDarkMode;
    if (isDarkMode) {
        document.body.classList.add('dark');
    } else {
        document.body.classList.remove('dark');
    }
});

switchMode.addEventListener('change', function () {
    if (this.checked) {
        document.body.classList.add('dark');
        localStorage.setItem('darkMode', 'true');
    } else {
        document.body.classList.remove('dark');
        localStorage.setItem('darkMode', 'false');
    }
});

let btn1 = document.querySelector('#btn1');
let btn2 = document.querySelector('#btn2');
let btn3 = document.querySelector('#btn3');
let btn4 = document.querySelector('#btn4');
let btn5 = document.querySelector('#btn5');
let btn6 = document.querySelector('#btn6');

let buzzer = document.querySelector('#buzzer');
let door = document.querySelector('#door');
let fan = document.querySelector('#fan');
let state1 = localStorage.getItem("state1") ? JSON.parse(localStorage.getItem("state1")) : false;
let state2 = localStorage.getItem("state2") ? JSON.parse(localStorage.getItem("state2")) : false;
let state3 = localStorage.getItem("state3") ? JSON.parse(localStorage.getItem("state3")) : false;
let state5 = localStorage.getItem("state5") ? JSON.parse(localStorage.getItem("state5")) : false;

let audioContext = new (window.AudioContext || window.webkitAudioContext)();
let oscillator = null;

function startAlertSound() {
    if (!oscillator) {
        oscillator = audioContext.createOscillator();
        oscillator.type = 'sine';
        oscillator.frequency.setValueAtTime(440, audioContext.currentTime);
        oscillator.connect(audioContext.destination);
        oscillator.start();
    }
}

function stopAlertSound() {
    if (oscillator) {
        oscillator.stop();
        oscillator = null;
    }
}

btn1.addEventListener('click', () => {
    state1 = true;
    buzzer.src = 'img/buzzer.png';
    firebase.database().ref("control").set({ 
        buzzer: state1,
        door: state2,
        fan: state3,
    });
    saveStates();
});

btn2.addEventListener('click', () => {
    state1 = false;
    buzzer.src = 'img/bell.png';
    firebase.database().ref("control").set({
        buzzer: state1,
        door: state2,
        fan: state3,
    });
    saveStates();
});    

btn3.addEventListener('click', () => {
    state2 = true;
    door.src = 'img/cua_mo.png';
    firebase.database().ref("control").set({
        buzzer: state1,
        door: state2,
        fan: state3,
    });
    saveStates();
});    

btn4.addEventListener('click', () => {
    state2 = false;
    door.src = 'img/cua_dong.png';
    firebase.database().ref("control").set({
        buzzer: state1,
        door: state2,
        fan: state3,
    });
    saveStates();
});    

btn5.addEventListener('click', () => {
    state3 = true;
    fan.src = 'img/fan_on.png';
    firebase.database().ref("control").set({
        buzzer: state1,
        door: state2,
        fan: state3,
    });
    saveStates();
});

btn6.addEventListener('click', () => {
    state3 = false;
    fan.src = 'img/fan_off.png';
    firebase.database().ref("control").set({
        buzzer: state1,
        door: state2,
        fan: state3,
    });
    saveStates();
});

let thresholdGas = 400;
let thresholdTemp = 30;
let isAlertActive = false;

const sensorRef = firebase.database().ref('sensor');
sensorRef.on('value', (snapshot) => {
    const data = snapshot.val();
    if (!data || typeof data.gas === 'undefined' || typeof data.temperature === 'undefined') {
        console.error("Firebase data missing or invalid:", data);
        return;
    }

    const gas = data.gas;
    const temp = data.temperature;

    const gasElem = document.getElementById("gas-current") || document.querySelector('.gas-current-value');
    const tempElem = document.getElementById("temp-current") || document.querySelector('.temp-current-value');

    if (!gasElem || !tempElem) {
        console.error("Display elements not found. Gas element:", gasElem, "Temp element:", tempElem);
        return;
    }

    gasElem.innerText = `${gas} ppm`;
    tempElem.innerText = `${temp}°C`;

    console.log(`Sensor values - Gas: ${gas}, Temp: ${temp}, Mode: ${document.getElementById("alarm-mode")?.innerText}`);

    const alarmMode = document.getElementById("alarm-mode")?.innerText.trim().toLowerCase() || 'manual';

    if (alarmMode === 'auto') {
    if (gas > thresholdGas || temp > thresholdTemp) {
        if (gas > thresholdGas) {
            gasElem.style.color = 'red';
        } else {
            gasElem.style.color = 'black';
        }

        if (temp > thresholdTemp) {
            tempElem.style.color = 'red';
        } else {
            tempElem.style.color = 'black';
        }

        if (!isAlertActive) {
            startAlertSound();
            isAlertActive = true;
            console.log("Alert sound started");
        }
        firebase.database().ref("control").set({
            fan: true,
            buzzer: true,
            door: true
        });
        state1 = true;
        state2 = true;
        state3 = true;
        buzzer.src = 'img/buzzer.png';
        door.src = 'img/cua_mo.png';
        fan.src = 'img/fan_on.png';
    } else {
        gasElem.style.color = 'black';
        tempElem.style.color = 'black';
        if (isAlertActive) {
            stopAlertSound();
            isAlertActive = false;
            console.log("Alert sound stopped");
        }
        firebase.database().ref("control").set({
            fan: false,
            buzzer: false,
            door: false
        });
        state1 = false;
        state2 = false;
        state3 = false;
        buzzer.src = 'img/bell.png';
        door.src = 'img/cua_dong.png';
        fan.src = 'img/fan_off.png';
    }
} else {
    gasElem.style.color = gas > thresholdGas ? 'red' : 'black';
    tempElem.style.color = temp > thresholdTemp ? 'red' : 'black';
    stopAlertSound();
    isAlertActive = false;
}
    saveStates(); 
});

function saveStates() {
    localStorage.setItem("state1", JSON.stringify(state1));
    localStorage.setItem("state2", JSON.stringify(state2));
    localStorage.setItem("state3", JSON.stringify(state3));
}

function restoreStates() {
    state1 = JSON.parse(localStorage.getItem("state1")) || false;
    state2 = JSON.parse(localStorage.getItem("state2")) || false;
    state3 = JSON.parse(localStorage.getItem("state3")) || false;

    buzzer.src = state1 ? 'img/buzzer.png' : 'img/bell.png';
    door.src = state2 ? 'img/cua_mo.png' : 'img/cua_dong.png';
    fan.src = state3 ? 'img/fan_on.png' : 'img/fan_off.png';
}
window.addEventListener("load", () => {
    restoreStates();
});