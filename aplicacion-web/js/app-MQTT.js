// Configuración de la conexión al broker HiveMQ mediante WebSockets
const clientId = 'WebApp_' + Math.random().toString(16).substr(2, 8);
const host = ENV.MQTT_HOST;

const options = {
    keepalive: 60,
    clientId: clientId,
    protocolId: 'MQTT',
    protocolVersion: 4,
    clean: true,
    reconnectPeriod: 1000,
    connectTimeout: 30 * 1000,
    username: ENV.MQTT_USER,
    password: ENV.MQTT_PASS
};

console.log('Conectando al broker MQTT...');
const client = mqtt.connect(host, options);

// Referencias
const Broker = document.getElementById('conexion-broker');
const NodeMCU = document.getElementById('conexion-nodemcu');
const contAlerta = document.getElementById('contenedor-alerta');
const txtAlerta = document.getElementById('val-alerta');

// Si nos conectamos al broker, actualizamos el estado y nos suscribimos a los topics.
client.on('connect', () => {
    console.log('Conectado exitosamente al broker HiveMQ');
    
    // Actualizamos el estado de conexión con el broker
    Broker.textContent = 'Broker: Conectado';
    Broker.classList.replace('bg-danger', 'bg-success');

    // nos suscribimos a los topics que establecimos en la arquitectura del proyecto
    client.subscribe('proyectoFinal/sala_servidores/sensores/temperatura');
    client.subscribe('proyectoFinal/sala_servidores/sensores/humedad');
    client.subscribe('proyectoFinal/sala_servidores/estado/ventilador');
    client.subscribe('proyectoFinal/sala_servidores/estado/modo');
    client.subscribe('proyectoFinal/sala_servidores/estado/conexion');
    client.subscribe('proyectoFinal/sala_servidores/eventos/alertas');
    client.subscribe('proyectoFinal/sala_servidores/config/umbral_temperatura');
    client.subscribe('proyectoFinal/sala_servidores/config/umbral_humedad');
});

// Recepción de mensajes para actualizar información
client.on('message', (topic, message) => {
    const msgStr = message.toString();
    console.log(`Mensaje recibido [${topic}]: ${msgStr}`);

    if (topic === 'proyectoFinal/sala_servidores/sensores/temperatura') {
        document.getElementById('val-temp').textContent = msgStr + ' °C';
    } 
    else if (topic === 'proyectoFinal/sala_servidores/sensores/humedad') {
        document.getElementById('val-hum').textContent = msgStr + ' %';
    }
    else if (topic === 'proyectoFinal/sala_servidores/estado/ventilador') {
        document.getElementById('val-vent').textContent = msgStr;
        // cambiar color del texto según el estado
        document.getElementById('val-vent').style.color = (msgStr === 'ON') ? '#00e5ff' : '#e0e0e0';
    }
    else if (topic === 'proyectoFinal/sala_servidores/estado/modo') {
        document.getElementById('val-modo').textContent = msgStr;
    }
    else if (topic === 'proyectoFinal/sala_servidores/eventos/alertas') {
        txtAlerta.textContent = 'Alertas: ' + msgStr;
        
        // Cambiamos el color de la alerta
        contAlerta.className = 'alert custom-alert text-center fw-bold mb-4'; // Reset
        if (msgStr.includes('ALERTA')) {
            contAlerta.classList.add('alert-danger'); 
        } else if (msgStr !== 'Normal') {
            contAlerta.classList.add('alert-warning'); 
        }
    }
    // Verificamos el Last Will para conocer el estado de conexión del NodeMCU
    else if (topic === 'proyectoFinal/sala_servidores/estado/conexion') {
        if (msgStr === 'Online') {
            NodeMCU.textContent = 'NodeMCU: Conectado';
            NodeMCU.classList.replace('bg-danger', 'bg-success');
        } else {
            NodeMCU.textContent = 'NodeMCU: Desconectado';
            NodeMCU.classList.replace('bg-success', 'bg-danger');
            
            // Limpiaamos los valores si se pierde la conexión
            document.getElementById('val-vent').textContent = '--';
            document.getElementById('val-temp').textContent = '-- °C';
            document.getElementById('val-hum').textContent = '-- %';
            document.getElementById('val-modo').textContent = '--';
        }
    }
    else if (topic === 'proyectoFinal/sala_servidores/config/umbral_temperatura') {
        document.getElementById('input-umbral-temperatura').value = msgStr;
    }
    //
    else if (topic === 'proyectoFinal/sala_servidores/config/umbral_humedad') {
        document.getElementById('input-umbral-humedad').value = msgStr;
    }
});

// Función para enviar comandos al NodeMCU
window.enviarComando = function(tipo, valor) {
    let topic = '';
    if (tipo === 'modo') {
        topic = 'proyectoFinal/sala_servidores/control/modo';
    } else if (tipo === 'ventilador') {
        topic = 'proyectoFinal/sala_servidores/control/ventilador';
    }
    
    if (client.connected) {
        client.publish(topic, valor, { qos: 1 });
        console.log(`Comando MQTT enviado: ${topic} -> ${valor}`);
    } else {
        alert('Error: No estás conectado al broker MQTT.');
    }
};

// Función para actualizar la configuración del umbral de forma remota
window.configurarUmbralTemperatura = function() {
    const valor = document.getElementById('input-umbral-temperatura').value;
    if (client.connected) {
        client.publish('proyectoFinal/sala_servidores/config/umbral_temperatura', valor, { qos: 1, retain: true });
        const input = document.getElementById('input-umbral-temperatura');
        input.classList.add('bg-success');
        setTimeout(() => input.classList.remove('bg-success'), 500);
    }
};

window.configurarUmbralHumedad = function() {
    const valor = document.getElementById('input-umbral-humedad').value;
    if (client.connected) {
        client.publish('proyectoFinal/sala_servidores/config/umbral_humedad', valor, { qos: 1, retain: true });
        const input = document.getElementById('input-umbral-humedad');
        input.classList.add('bg-success');
        setTimeout(() => input.classList.remove('bg-success'), 500);
    }
};