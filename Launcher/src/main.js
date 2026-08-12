const { invoke } = window.__TAURI__.core;
const { listen } = window.__TAURI__.event;

document.addEventListener("DOMContentLoaded", () => {
    const btnPlay = document.getElementById("btn-play");
    const progressBar = document.getElementById("progress-bar");
    const statusText = document.getElementById("status-text");
    const percentageText = document.getElementById("percentage-text");
    const newsList = document.getElementById("news-list");
    const btnClose = document.getElementById("close-btn");
    const statusDot = document.getElementById("status-dot");
    const serverStatusText = document.getElementById("server-status-text");

    // Botón cerrar launcher
    btnClose.addEventListener("click", () => {
        invoke("close_launcher");
    });

    // Llenar noticias desde el backend
    invoke("fetch_news")
        .then((news) => {
            if (news.length === 0) {
                newsList.innerHTML = '<div style="text-align:center;color:#666;margin-top:20px;">No news published yet.</div>';
                return;
            }
            newsList.innerHTML = "";
            news.forEach(item => {
                const div = document.createElement("div");
                div.className = "news-item";
                div.innerHTML = `
                    <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 4px;">
                        <div class="news-title" data-lang-en="${item.title_en}" data-lang-es="${item.title_es}">${item.title_en}</div>
                        <button class="lang-btn" data-current-lang="en">ESPAÑOL</button>
                    </div>
                    <div class="news-date">POSTED BY <span style="color:#fff">${item.author}</span> ON ${item.created_at}</div>
                    <div class="news-desc" data-lang-en="${item.content_en}" data-lang-es="${item.content_es}">${item.content_en.replace(/\n/g, '<br>')}</div>
                `;
                newsList.appendChild(div);

                const langBtn = div.querySelector('.lang-btn');
                const titleEl = div.querySelector('.news-title');
                const contentEl = div.querySelector('.news-desc');
                
                langBtn.addEventListener('click', () => {
                    let currentLang = langBtn.getAttribute('data-current-lang');
                    if (currentLang === 'en') {
                        titleEl.textContent = titleEl.getAttribute('data-lang-es');
                        contentEl.innerHTML = contentEl.getAttribute('data-lang-es').replace(/\n/g, '<br>');
                        langBtn.textContent = 'ENGLISH';
                        langBtn.setAttribute('data-current-lang', 'es');
                    } else {
                        titleEl.textContent = titleEl.getAttribute('data-lang-en');
                        contentEl.innerHTML = contentEl.getAttribute('data-lang-en').replace(/\n/g, '<br>');
                        langBtn.textContent = 'ESPAÑOL';
                        langBtn.setAttribute('data-current-lang', 'en');
                    }
                });
            });
        })
        .catch((err) => {
            newsList.innerHTML = `<div style="text-align:center;color:#a32c2c;margin-top:20px;">Error loading news: ${err}</div>`;
        });

    // Escuchar eventos de progreso desde Rust
    listen("update-progress", (event) => {
        const payload = event.payload;
        
        statusText.textContent = payload.message.toUpperCase();
        percentageText.textContent = `${payload.percentage}% COMPLETED`;
        progressBar.style.width = `${payload.percentage}%`;

        if (payload.is_ready) {
            btnPlay.disabled = false;
        }
    });

    // Evento del botón Jugar
    btnPlay.addEventListener("click", () => {
        if (!btnPlay.disabled) {
            btnPlay.disabled = true;
            statusText.textContent = "LAUNCHING GAME...";
            invoke("launch_game")
                .then(() => {
                    // El launcher se cierra desde Rust
                })
                .catch((err) => {
                    statusText.textContent = "ERROR: UPDATE FAILED";
                    btnPlay.disabled = false;
                });
        }
    });

    // Iniciar el proceso de actualización al abrir el launcher
    invoke("start_update_process").catch((err) => {
        statusText.textContent = "CRITICAL ERROR: UPDATE FAILED";
    });

    // Función para actualizar el estado del servidor
    function updateServerStatus() {
        invoke("fetch_server_status")
            .then((status) => {
                if (status.status === "online") {
                    statusDot.style.background = "#2ca336";
                    statusDot.style.boxShadow = "0 0 5px #2ca336";
                    serverStatusText.innerHTML = `PLAYERS ONLINE: <span class="online">${status.players}</span>`;
                } else if (status.status === "shutdown") {
                    statusDot.style.background = "#d4a322";
                    statusDot.style.boxShadow = "0 0 5px #d4a322";
                    serverStatusText.innerHTML = `RESTARTING IN: <span class="shutdown">${status.countdown}s</span>`;
                } else {
                    statusDot.style.background = "var(--red-offline)";
                    statusDot.style.boxShadow = "0 0 5px var(--red-offline)";
                    serverStatusText.innerHTML = `PLAYERS ONLINE: <span class="offline">OFFLINE</span>`;
                }
            })
            .catch((err) => {
                console.error("Error fetching server status:", err);
                statusDot.style.background = "var(--red-offline)";
                statusDot.style.boxShadow = "0 0 5px var(--red-offline)";
                serverStatusText.innerHTML = `PLAYERS ONLINE: <span class="offline">OFFLINE</span>`;
            });
    }

    // Llamada inicial y luego cada 5 segundos
    updateServerStatus();
    setInterval(updateServerStatus, 5000);
});
