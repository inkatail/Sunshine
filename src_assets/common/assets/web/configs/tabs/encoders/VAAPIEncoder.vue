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
    <!-- Async Depth -->
    <div class="mb-3">
      <label for="vaapi_async_depth" class="form-label">{{ $t('config.vaapi_async_depth') }}</label>
      <input type="number" min="1" max="64" class="form-control" id="vaapi_async_depth" placeholder="1"
             v-model="config.vaapi_async_depth" />
      <div class="form-text">
        {{ $t('config.vaapi_async_depth_desc') }}
      </div>
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
