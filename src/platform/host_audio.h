#ifndef HOST_AUDIO_H
#define HOST_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Base assets directory (same as CUPCAKE_ASSETS / "assets"). */
int host_audio_init(const char *assets_base);
void host_audio_shutdown(void);

/* Play by game.model id; "stop" halts all channels/voices. */
int host_audio_play(const char *sfx_id);

/* JS AcclaimSuperplayDevice.onSound — toggle handheld mute. */
void host_audio_toggle_mute(void);
int host_audio_is_muted(void);

/* Odroid backend: mix active voices and submit PCM (no-op on SDL_mixer builds). */
void host_audio_pump(int frame_count);

#ifdef __cplusplus
}
#endif

#endif
