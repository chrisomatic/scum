bool audio_init();
void audio_deinit();

void audio_set_listener_pos(float x, float y, float z);

// Audio Buffer
int audio_load_file(char* filepath);

// Source
int audio_source_create();
void audio_source_update_position(int src, float x, float y, float z);
void audio_source_assign_buffer(int source, int buffer);
void audio_source_play(int source);

bool audio_source_is_playing(int source);
