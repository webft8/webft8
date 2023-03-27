

export function add_msg(webft8_msg) {
    var table = document.getElementById("webft8_messages");
    let color = 'black';
    if(webft8_msg.text.startsWith("CQ")) {
        color = 'green';
    }
    if(webft8_msg.text.endsWith(" RR73") || webft8_msg.text.endsWith(" 73")) {
        color = 'blue'
    }
    if(webft8_msg.text.includes("+") || webft8_msg.text.includes("-")){
        color = 'magenta';
    }
    var row = table.insertRow(1);
    row.insertCell(0).innerHTML = webft8_msg.ts.toLocaleTimeString();
    row.insertCell(1).innerHTML = webft8_msg.snr;
    row.insertCell(2).innerHTML = `<span style="color: ${color};">${webft8_msg.text}</span>` ;
    row.insertCell(3).innerHTML = webft8_msg.freq_hz;
}
