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
    <!-- Rate Control Mode -->
    <div class="mb-3">
      <label for="vaapi_rc_mode" class="form-label">{{ $t('config.vaapi_rc_mode') }}</label>
      <select id="vaapi_rc_mode" class="form-select" v-model="config.vaapi_rc_mode">
        <option value="auto">{{ $t('config.vaapi_rc_mode_auto') }}</option>
        <option value="cqp">{{ $t('config.vaapi_rc_mode_cqp') }}</option>
        <option value="cbr">{{ $t('config.vaapi_rc_mode_cbr') }}</option>
        <option value="vbr">{{ $t('config.vaapi_rc_mode_vbr') }}</option>
      </select>
      <div class="form-text">{{ $t('config.vaapi_rc_mode_desc') }}</div>
    </div>

    <!-- Strict RC Buffer -->
    <Checkbox class="mb-3"
              id="vaapi_strict_rc_buffer"
              locale-prefix="config"
              v-model="config.vaapi_strict_rc_buffer"
              default="false"
    ></Checkbox>
  </div>
</template>

<style scoped>

</style>
