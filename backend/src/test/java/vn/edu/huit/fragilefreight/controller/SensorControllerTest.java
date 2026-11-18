package vn.edu.huit.fragilefreight.controller;

import static org.mockito.Mockito.when;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.webmvc.test.autoconfigure.WebMvcTest;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.request.MockMvcRequestBuilders;
import org.springframework.test.web.servlet.result.MockMvcResultMatchers;
import org.springframework.test.context.bean.override.mockito.MockitoBean;
import vn.edu.huit.fragilefreight.model.SensorState;
import vn.edu.huit.fragilefreight.service.SensorStateService;

@WebMvcTest(controllers = SensorController.class, properties = "mqtt.enabled=false")
class SensorControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockitoBean
    private SensorStateService sensorStateService;

    @Test
    void getLatestSensorState_shouldReturnLatestStateWhenPresent() throws Exception {
        SensorState sensorState = new SensorState("esp32-01", 0.02, -0.01, 1.00, 1.00, "SAFE");
        when(sensorStateService.getLatestState()).thenReturn(java.util.Optional.of(sensorState));

        mockMvc.perform(MockMvcRequestBuilders.get("/api/sensor/latest")
                .accept(MediaType.APPLICATION_JSON))
            .andExpect(MockMvcResultMatchers.status().isOk())
            .andExpect(MockMvcResultMatchers.jsonPath("$.deviceId").value("esp32-01"))
            .andExpect(MockMvcResultMatchers.jsonPath("$.status").value("SAFE"));
    }

    @Test
    void getAlertReturnsSeparateLatestAlert() throws Exception {
        when(sensorStateService.getLatestAlert("esp32-01"))
            .thenReturn(java.util.Optional.of(new SensorState("esp32-01", 0, 0, 0, 2.5, "DANGER")));
        mockMvc.perform(MockMvcRequestBuilders.get("/api/sensor/esp32-01/alert"))
            .andExpect(MockMvcResultMatchers.status().isOk())
            .andExpect(MockMvcResultMatchers.jsonPath("$.gForce").value(2.5))
            .andExpect(MockMvcResultMatchers.jsonPath("$.status").value("DANGER"))
            .andExpect(MockMvcResultMatchers.jsonPath("$.deviceId").value("esp32-01"))
            .andExpect(MockMvcResultMatchers.jsonPath("$.ax").doesNotExist());
    }
}
