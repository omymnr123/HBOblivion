use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::path::PathBuf;
use tauri::{AppHandle, Emitter};
use tokio::io::AsyncReadExt;

#[derive(Serialize, Deserialize, Debug)]
struct ServerStatus {
    status: String,
    players: u32,
    countdown: u32,
}

#[derive(Serialize, Deserialize, Debug)]
struct NewsItem {
    id: u32,
    author: String,
    title_en: String,
    content_en: String,
    title_es: String,
    content_es: String,
    created_at: String,
}

#[derive(Serialize, Deserialize, Clone)]
struct ProgressPayload {
    message: String,
    percentage: u32,
    is_ready: bool,
}

#[derive(Deserialize, Debug)]
struct Version {
    major: u32,
    minor: u32,
    patch: u32,
}

#[derive(Deserialize, Debug)]
struct FileInfo {
    path: String,
    sha256: String,
    size: u64,
    executable: bool,
    platform: String,
}

#[derive(Deserialize, Debug)]
struct Manifest {
    version: Version,
    files: Vec<FileInfo>,
}

#[tauri::command]
async fn start_update_process(app: AppHandle) -> Result<(), String> {
    let _ = app.emit("update-progress", ProgressPayload {
        message: "Connecting to server...".to_string(),
        percentage: 0,
        is_ready: false,
    });

    let manifest_url = "http://127.0.0.1:8081/update.manifest.json";
    let base_url = "http://127.0.0.1:8081";

    let manifest: Manifest = reqwest::get(manifest_url)
        .await.map_err(|e| e.to_string())?
        .json()
        .await.map_err(|e| e.to_string())?;

    let total_files = manifest.files.len();
    let mut files_to_download = Vec::new();

    for (index, file_info) in manifest.files.iter().enumerate() {
        let pct = (index as f32 / total_files as f32 * 50.0) as u32;
        let _ = app.emit("update-progress", ProgressPayload {
            message: format!("Comprobando {}...", file_info.path),
            percentage: pct,
            is_ready: false,
        });

        let mut needs_download = true;
        let file_path = PathBuf::from(&file_info.path);
        
        if file_path.exists() {
            if let Ok(mut file) = tokio::fs::File::open(&file_path).await {
                let mut hasher = Sha256::new();
                let mut buffer = [0; 8192];
                while let Ok(n) = file.read(&mut buffer).await {
                    if n == 0 { break; }
                    hasher.update(&buffer[..n]);
                }
                let hash = format!("{:x}", hasher.finalize());
                if hash == file_info.sha256 {
                    needs_download = false;
                }
            }
        }

        if needs_download {
            files_to_download.push(file_info);
        }
    }

    let download_count = files_to_download.len();
    for (index, file_info) in files_to_download.iter().enumerate() {
        let pct = 50 + (index as f32 / download_count as f32 * 50.0) as u32;
        let _ = app.emit("update-progress", ProgressPayload {
            message: format!("Downloading {}...", file_info.path),
            percentage: pct,
            is_ready: false,
        });

        let file_url = format!("{}/{}", base_url, file_info.path);
        let response = reqwest::get(&file_url).await.map_err(|e| e.to_string())?;
        
        let file_path = PathBuf::from(&file_info.path);
        if let Some(parent) = file_path.parent() {
            tokio::fs::create_dir_all(parent).await.map_err(|e| e.to_string())?;
        }

        let bytes = response.bytes().await.map_err(|e| e.to_string())?;
        
        match tokio::fs::write(&file_path, &bytes).await {
            Ok(_) => {},
            Err(e) => {
                // On Windows, the running executable is locked and cannot be overwritten.
                // We move it to an 'updates' folder so we don't accumulate .old files.
                if let Some(parent) = file_path.parent() {
                    let updates_dir = parent.join("updates");
                    let _ = tokio::fs::create_dir_all(&updates_dir).await;
                    
                    if let Some(file_name) = file_path.file_name() {
                        let backup_path = updates_dir.join(file_name);
                        
                        // Delete the old backup if it exists (it's no longer running, so Windows allows this)
                        let _ = tokio::fs::remove_file(&backup_path).await;
                        
                        // Move the currently running executable to the updates folder
                        if tokio::fs::rename(&file_path, &backup_path).await.is_ok() {
                            tokio::fs::write(&file_path, &bytes).await.map_err(|err| format!("Failed to write {}: {}", file_path.display(), err))?;
                        } else {
                            return Err(format!("Failed to write {}: {}", file_path.display(), e));
                        }
                    } else {
                        return Err(format!("Failed to write {}: {}", file_path.display(), e));
                    }
                } else {
                    return Err(format!("Failed to write {}: {}", file_path.display(), e));
                }
            }
        }
    }

    let _ = app.emit("update-progress", ProgressPayload {
        message: "Updated successfully.".to_string(),
        percentage: 100,
        is_ready: true,
    });

    Ok(())
}

#[tauri::command]
async fn launch_game() -> Result<(), String> {
    // Lanzar el ejecutable que se encuentra en la misma carpeta que el launcher
    std::process::Command::new("Game_x64_win.exe")
        .spawn()
        .map_err(|e| format!("ERROR LAUNCHING GAME: {}", e))?;
    std::process::exit(0);
}

#[tauri::command]
fn close_launcher() {
    std::process::exit(0);
}

#[tauri::command]
async fn fetch_server_status() -> Result<ServerStatus, String> {
    // URL de la API web publica
    let url = "http://127.0.0.1/hboblivion/api_server_status.php";
    
    let status: ServerStatus = match reqwest::get(url).await {
        Ok(resp) => match resp.json().await {
            Ok(s) => s,
            Err(e) => return Err(e.to_string()),
        },
        Err(e) => return Err(e.to_string()),
    };

    Ok(status)
}

#[tauri::command]
async fn fetch_news() -> Result<Vec<NewsItem>, String> {
    let url = "http://127.0.0.1/hboblivion/api_news.php";
    
    let news: Vec<NewsItem> = match reqwest::get(url).await {
        Ok(resp) => match resp.json().await {
            Ok(n) => n,
            Err(e) => return Err(e.to_string()),
        },
        Err(e) => return Err(e.to_string()),
    };

    Ok(news)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![
            start_update_process, 
            launch_game,
            fetch_server_status,
            fetch_news,
            close_launcher
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
