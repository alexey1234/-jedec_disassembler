let jedutilModule = null;

// Переменные для накопления текста должны быть глобальными для app.js
let stdoutAccumulator = "";
let stderrAccumulator = "";

// Инициализируем перехват ДО того, как jedutil.js начнет использовать Module
var Module = {
    locateFile: function(path, prefix) {
        if (path.endsWith('.wasm')) return './' + path;
        return prefix + path;
    },
    // Перехватываем стандартный вывод Си-кода на самом глубоком уровне
    print: function(text) {
        stdoutAccumulator += text + "\n";
    },
    printErr: function(text) {
        stderrAccumulator += text + "\n";
    },
    onRuntimeInitialized: function() {
        jedutilModule = Module;
        const badge = document.getElementById('statusBadge');
        badge.className = "badge badge-ready";
        badge.innerHTML = '<span class="dot dot-ready"></span><span>WASM ready</span>';
        document.getElementById('terminalConsole').innerHTML = "<span class='log-info'>// System ready. Paste .JED text into the left window and click the button.</span>";
        checkInputs();
    }
};

document.getElementById('jedTextInput').addEventListener('input', checkInputs);

function checkInputs() {
    const textLength = document.getElementById('jedTextInput').value.trim().length;
    document.getElementById('btnDecompile').disabled = !(textLength > 0 && jedutilModule !== null);
}

function printLog(text, isError = false) {
    const consoleDiv = document.getElementById('terminalConsole');
    if (consoleDiv.innerText.includes("// waiting") || consoleDiv.innerText.includes("// System ready")) {
        consoleDiv.innerHTML = "";
    }
    const span = document.createElement('span');
    span.className = isError ? "log-error" : "";
    span.innerText = text;
    consoleDiv.appendChild(span);
    consoleDiv.scrollTop = consoleDiv.scrollHeight;
}

document.getElementById('btnClear').addEventListener('click', () => {
    document.getElementById('terminalConsole').innerHTML = "<span class='log-info'>// Output cleared</span>";
});

document.getElementById('btnDecompile').addEventListener('click', () => {
    if (!jedutilModule) return;

    const jedTextInput = document.getElementById('jedTextInput').value;
    if (!jedTextInput.trim()) {
        printLog("[Error]: JED file input area is empty..", true);
        return;
    }

    // === 1. ПОДГОТОВКА СТРУКТУРЫ JEDEC ===
    let cleanJedText = jedTextInput.trim();
    let formattedJedText = "\x02" + cleanJedText + "\x03";
    const encoder = new TextEncoder();
    const jedData = encoder.encode(formattedJedText);
    
    const virtualFileName = "input_firmware.jed";
    jedutilModule.FS.writeFile(virtualFileName, jedData);

    const targetDevice = document.getElementById('pldTarget').value || "GAL16V8";
    
    // Очищаем экран вывода на веб-странице
    document.getElementById('terminalConsole').innerHTML = "";
    
    // Выводим красивую информационную строчку вызова нашей новой Си-команды
    printLog("$ jedutil -viewandcompatible " + virtualFileName + " " + targetDevice + "\n\n");

    // Очищаем аккумуляторы перед запуском
    stdoutAccumulator = "";
    stderrAccumulator = "";

    // === 2. ВЫЗОВ НАШЕЙ КОМБИНИРОВАННОЙ СИ-КОМАНДЫ ===
    try {
        // Вызываем всего один раз — Си-код сам сделает всю работу!
        jedutilModule.callMain(['-viewandcompatible', virtualFileName, targetDevice]);
    } catch (e) {
        // Ловим системное завершение утилиты
    }

    // === 3. ВЫВОД НА ЭКРАН СТРАНИЦЫ ===
    let fullOutput = stdoutAccumulator + stderrAccumulator;

    if (fullOutput.trim()) {
        printLog(fullOutput);
    } else {
        printLog("[Error]: The MAME parser returned an empty result. Ensure that the text has been copied in its entirety.", true);
    }
});

