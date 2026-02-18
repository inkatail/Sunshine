<script setup>
import { ref } from 'vue'
import Checkbox from "../../../Checkbox.vue";
import Select from "../../../Select.vue";

const props = defineProps([
  'platform',
  'config',
])

const config = ref(props.config)
</script>

<template>
  <div id="vaapi-encoder" class="config-page">
    <!-- RC Mode -->
    <Select class="mb-3"
            id="vaapi_rc_mode"
            locale-prefix="config"
            v-model="config.vaapi_rc_mode"
            :options="[
                { value: 'auto', label: 'vaapi_rc_mode_auto' },
                { value: 'cqp', label: 'vaapi_rc_mode_cqp' },
                { value: 'vbr', label: 'vaapi_rc_mode_vbr' },
                { value: 'cbr', label: 'vaapi_rc_mode_cbr' }
            ]"
    ></Select>

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
