<script setup>
import { ref } from 'vue'
import Checkbox from "../../../Checkbox.vue";

const props = defineProps([
  'platform',
  'config',
])

const config = ref(props.config)
</script>

<template>
  <div id="vaapi-encoder" class="config-page">
    <!-- VAAPI Rate Control -->
    <div class="mb-3">
      <label for="vaapi_rc_mode" class="form-label">{{ $t('config.vaapi_rc') }}</label>
      <select id="vaapi_rc_mode" class="form-select" v-model="config.vaapi_rc_mode">
        <option value="auto">{{ $t('config.vaapi_rc_auto') }}</option>
        <option value="cbr">{{ $t('config.vaapi_rc_cbr') }}</option>
        <option value="vbr">{{ $t('config.vaapi_rc_vbr') }}</option>
        <option value="cqp">{{ $t('config.vaapi_rc_cqp') }}</option>
      </select>
      <div class="form-text">{{ $t('config.vaapi_rc_desc') }}</div>
    </div>

    <!-- Strict RC Buffer -->
    <Checkbox class="mb-3"
              id="vaapi_strict_rc_buffer"
              locale-prefix="config"
              v-model="config.vaapi_strict_rc_buffer"
              default="false"
    ></Checkbox>

    <!-- Performance Options -->
    <div class="mb-3 accordion">
      <div class="accordion-item">
        <h2 class="accordion-header">
          <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                  data-bs-target="#vaapi-performance">
            {{ $t('config.vaapi_performance_group') }}
          </button>
        </h2>
        <div id="vaapi-performance" class="accordion-collapse collapse">
          <div class="accordion-body">
            <!-- Async Depth -->
            <div class="mb-3">
              <label for="vaapi_async_depth" class="form-label">{{ $t('config.vaapi_async_depth') }}</label>
              <input type="number" class="form-control" id="vaapi_async_depth" 
                     v-model="config.vaapi_async_depth" min="1" max="64" placeholder="1">
              <div class="form-text">{{ $t('config.vaapi_async_depth_desc') }}</div>
            </div>

            <!-- Low Power Mode -->
            <div class="mb-3">
              <label for="vaapi_low_power_mode" class="form-label">{{ $t('config.vaapi_low_power_mode') }}</label>
              <select id="vaapi_low_power_mode" class="form-select" v-model="config.vaapi_low_power_mode">
                <option value="auto">{{ $t('config.vaapi_low_power_auto') }}</option>
                <option value="enabled">{{ $t('config.vaapi_low_power_enabled') }}</option>
                <option value="disabled">{{ $t('config.vaapi_low_power_disabled') }}</option>
              </select>
              <div class="form-text">{{ $t('config.vaapi_low_power_mode_desc') }}</div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- AMD Bitrate Control Mitigations -->
    <div class="mb-3 accordion">
      <div class="accordion-item">
        <h2 class="accordion-header">
          <button class="accordion-button collapsed" type="button" data-bs-toggle="collapse"
                  data-bs-target="#vaapi-amd-mitigations">
            {{ $t('config.vaapi_amd_mitigations_group') }}
          </button>
        </h2>
        <div id="vaapi-amd-mitigations" class="accordion-collapse collapse">
          <div class="accordion-body">
            <!-- QP Min -->
            <div class="mb-3">
              <label for="vaapi_qp_min" class="form-label">{{ $t('config.vaapi_qp_min') }}</label>
              <input type="number" class="form-control" id="vaapi_qp_min" 
                     v-model="config.vaapi_qp_min" min="0" max="51" 
                     :placeholder="$t('config.vaapi_qp_min_placeholder')">
              <div class="form-text">{{ $t('config.vaapi_qp_min_desc') }}</div>
            </div>

            <!-- QP Max -->
            <div class="mb-3">
              <label for="vaapi_qp_max" class="form-label">{{ $t('config.vaapi_qp_max') }}</label>
              <input type="number" class="form-control" id="vaapi_qp_max" 
                     v-model="config.vaapi_qp_max" min="0" max="51"
                     :placeholder="$t('config.vaapi_qp_max_placeholder')">
              <div class="form-text">{{ $t('config.vaapi_qp_max_desc') }}</div>
            </div>

            <!-- Enforce HRD -->
            <Checkbox class="mb-3"
                      id="vaapi_enforce_hrd"
                      locale-prefix="config"
                      v-model="config.vaapi_enforce_hrd"
                      default="false"
            ></Checkbox>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>

</style>
