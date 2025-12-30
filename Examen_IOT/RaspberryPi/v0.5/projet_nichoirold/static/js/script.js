let onlyFavs = false;
let currentFilterId = null;

function showView(id) {
    document.querySelectorAll('.view-section').forEach(v => v.style.display = 'none');
    const target = document.getElementById(id);
    if(target) target.style.display = 'block';
    
    document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));
    const navLink = document.getElementById('nav-' + id);
    if(navLink) navLink.classList.add('active');
    
    if(id === 'gallery' && !currentFilterId) renderPhotos();
    else if (id === 'gallery' && currentFilterId) renderPhotos();
}

function selectNichoir(id, batt, auto) {
    document.getElementById('detail-panel').style.display = 'block';
    document.getElementById('det-title').innerText = "Nichoir " + id;
    document.getElementById('det-auto').innerText = auto;
    
    // CORRECTION BATTERIE MAX 100%
    let val = parseFloat(batt);
    let pct = 0;
    let displayTxt = "--";

    if (!isNaN(val)) {
        if (val > 12) {
            pct = val;
            displayTxt = Math.round(val) + "%";
        } else {
            // 4.2V = 100%
            pct = ((val - 3.3) / (4.2 - 3.3)) * 100;
            displayTxt = Math.round(Math.min(100, pct)) + "% (" + val.toFixed(2) + "V)";
        }
    }
    // Clamp visuel entre 0 et 100
    pct = Math.min(100, Math.max(0, pct));
    
    document.getElementById('det-val').innerText = displayTxt;
    document.getElementById('det-bar').style.width = pct + "%";

    const btn = document.getElementById('btn-see-gallery');
    btn.onclick = function() {
        currentFilterId = id; 
        showView('gallery');
    };

    document.querySelectorAll('.nichoir-card').forEach(c => c.classList.remove('active'));
    event.currentTarget.classList.add('active');
}

function closePanel() {
    document.getElementById('detail-panel').style.display = 'none';
    document.querySelectorAll('.nichoir-card').forEach(c => c.classList.remove('active'));
}

function resetFilter() {
    currentFilterId = null;
    renderPhotos();
}

function toggleFavFilter() {
    onlyFavs = !onlyFavs;
    const btn = document.getElementById('fav-btn');
    btn.innerHTML = onlyFavs ? '<i class="fas fa-heart"></i> Voir Tout' : '<i class="far fa-heart"></i> Voir Favoris';
    btn.classList.toggle('active');
    renderPhotos();
}

// --- GESTION LIGHTBOX (ZOOM) ---
function openLightbox(url, date) {
    const lb = document.getElementById('lightbox');
    const img = document.getElementById('lightbox-img');
    const cap = document.getElementById('lightbox-caption');
    lb.style.display = "block";
    img.src = url;
    cap.innerText = date;
}

function closeLightbox() {
    document.getElementById('lightbox').style.display = "none";
}

function renderPhotos() {
    const grid = document.getElementById('grid');
    grid.innerHTML = '';

    const title = document.getElementById('gallery-title');
    const resetBtn = document.getElementById('reset-filter-btn');

    if(currentFilterId) {
        title.innerText = "Galerie - Nichoir " + currentFilterId;
        resetBtn.style.display = 'inline-block';
    } else {
        title.innerText = "Galerie Globale";
        resetBtn.style.display = 'none';
    }

    let data = currentFilterId ? photos.filter(p => p.nichoir_id == currentFilterId) : photos;
    if(onlyFavs) data = data.filter(p => p.fav);

    if(data.length === 0) {
        grid.innerHTML = '<p style="grid-column: 1/-1; text-align: center; color: #666; margin-top: 50px;">Aucune photo.</p>';
        return;
    }

    data.forEach(p => {
        const div = document.createElement('div');
        div.className = 'photo-card';
        div.innerHTML = `
            <img src="${p.url}" loading="lazy" style="cursor:zoom-in;" onclick="openLightbox('${p.url}', '${p.date}')">
            <div class="photo-info">
                <div style="display:flex; flex-direction:column;">
                    <span style="font-weight:bold; color: white;">Nichoir ${p.nichoir_id}</span>
                    <span style="font-size: 0.8rem; color: #ccc;">${p.date}</span>
                </div>
                <i id="fav-icon-${p.id}" class="${p.fav ? 'fas fa-heart active' : 'far fa-heart'}" onclick="toggleFav(${p.id})"></i>
            </div>
        `;
        grid.appendChild(div);
    });
}

async function toggleFav(id) {
    try {
        const res = await fetch(`/api/fav/${id}`, {method: 'POST'});
        if(res.ok) {
            const photo = photos.find(p => p.id === id);
            if(photo) {
                photo.fav = !photo.fav;
                const icon = document.getElementById(`fav-icon-${id}`);
                if(icon) icon.className = photo.fav ? 'fas fa-heart active' : 'far fa-heart';
                if(onlyFavs && !photo.fav) renderPhotos();
            }
        }
    } catch(e) { console.error(e); }
}
