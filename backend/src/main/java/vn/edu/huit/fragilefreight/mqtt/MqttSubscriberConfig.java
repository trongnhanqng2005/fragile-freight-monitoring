package vn.edu.huit.fragilefreight.mqtt;

import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Component;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import vn.edu.huit.fragilefreight.model.SensorState;
import vn.edu.huit.fragilefreight.service.SensorStateService;

@Component
@ConditionalOnProperty(name = "mqtt.enabled", havingValue = "true", matchIfMissing = true)
public class MqttSubscriberConfig {
    private static final Logger LOGGER = LoggerFactory.getLogger(MqttSubscriberConfig.class);
    private static final String TELEMETRY = "fragile-freight/esp32-01/telemetry";
    private static final String ALERT = "fragile-freight/esp32-01/alert";
    private static final String[] TOPICS = {TELEMETRY, ALERT};
    private final SensorStateService state;
    private final String brokerUrl;
    private final String clientId;
    private final ScheduledExecutorService retry = Executors.newSingleThreadScheduledExecutor();
    private volatile MqttClient client;
    private volatile boolean subscribed;

    public MqttSubscriberConfig(SensorStateService state,
        @Value("${mqtt.broker-url:tcp://localhost:1883}") String brokerUrl,
        @Value("${mqtt.client-id:fragile-freight-backend}") String clientId) {
        this.state = state;
        this.brokerUrl = brokerUrl;
        this.clientId = clientId;
    }

    @PostConstruct
    void start() {
        retry.scheduleWithFixedDelay(this::connectIfNeeded, 0, 5, TimeUnit.SECONDS);
    }

    private synchronized void connectIfNeeded() {
        if (retry.isShutdown()) return;
        try {
            if (client == null) {
                client = new MqttClient(brokerUrl, clientId, new MemoryPersistence());
                client.setTimeToWait(6000);
                client.setCallback(new MqttCallback() {
                    @Override public void connectionLost(Throwable cause) {
                        subscribed = false;
                        LOGGER.warn("MQTT connection lost; retry scheduled");
                    }
                    @Override public void messageArrived(String topic, MqttMessage message) {
                        receive(topic, message);
                    }
                    @Override public void deliveryComplete(IMqttDeliveryToken token) { }
                });
            }
            MqttConnectOptions options = new MqttConnectOptions();
            options.setCleanSession(true);
            options.setAutomaticReconnect(false);
            options.setConnectionTimeout(5);
            if (!client.isConnected()) {
                client.connect(options);
                subscribed = false;
            }
            if (!subscribed) {
                client.subscribe(TOPICS, new int[] {0, 0});
                subscribed = true;
                LOGGER.info("Connected to MQTT broker and subscribed to contract topics");
            }
        } catch (MqttException ex) {
            LOGGER.warn("MQTT unavailable; retrying in five seconds");
        }
    }

    private void receive(String topic, MqttMessage message) {
        if (message.isRetained() || (!TELEMETRY.equals(topic) && !ALERT.equals(topic))) return;
        try {
            String payload = StandardCharsets.UTF_8.newDecoder()
                .onMalformedInput(CodingErrorAction.REPORT)
                .onUnmappableCharacter(CodingErrorAction.REPORT)
                .decode(ByteBuffer.wrap(message.getPayload())).toString();
            if (TELEMETRY.equals(topic)) state.updateTelemetry(MqttMessageParser.parseTelemetry(payload));
            else {
                SensorState alert = MqttMessageParser.parseAlert(payload);
                state.updateAlert(alert.getDeviceId(), alert.getGForce());
            }
        } catch (CharacterCodingException | IllegalArgumentException ex) {
            LOGGER.warn("Ignoring malformed MQTT message on {}", topic);
        }
    }

    @PreDestroy
    synchronized void stop() {
        retry.shutdownNow();
        MqttClient current = client;
        if (current != null) {
            try {
                if (current.isConnected()) current.disconnect();
                current.close();
            } catch (MqttException ex) {
                LOGGER.warn("Unable to close MQTT client cleanly");
            }
        }
    }
}
