let onlyFavs = false;

function showView(id) {
    // Masquer toutes les sections
    document.querySelectorAll('.view-section').forEach(v => v.style.display = 'none');
    // Afficher la demandée
    const target = document.getElementById(id);
    if(target) target.style.display = 'block';
    
    // Gérer le menu actif
    document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));
    const navLink = document.getElementById('nav-' + id);
    if(navLink) navLink.classList.add('active');
    
    if(id === 'gallery') renderPhotos();
}

function selectNichoir(id, batt, auto) {
    document.getElementById('detail-panel').style.display = 'block';
    document.getElementById('det-title').innerText = "Nichoir " + id;
    document.getElementById('det-auto').innerText = auto;
    
    // Modification pour afficher le voltage brut
    document.getElementById('det-val').innerText = batt; 
    
    // Barre de progression (On convertit grossièrement le Voltage en % juste pour la barre visuelle)
    // 3.2V = 0%, 4.2V = 100%
    let v = parseFloat(batt);
    let pct = ((v - 3.2) / (4.2 - 3.2)) * 100;
    if(pct < 0) pct = 0; if(pct > 100) pct = 100;
    
    document.getElementById('det-bar').style.width = pct + "%";

    document.querySelectorAll('.nichoir-card').forEach(c => c.classList.remove('active'));
    event.currentTarget.classList.add('active');
}

function closePanel() {
    document.getElementById('detail-panel').style.display = 'none';
    document.querySelectorAll('.nichoir-card').forEach(c => c.classList.remove('active'));
}

function toggleFavFilter() {
    onlyFavs = !onlyFavs;
    const btn = document.getElementById('fav-btn');
    btn.innerHTML = onlyFavs ? '<i class="fas fa-heart"></i> Voir Tout' : '<i class="far fa-heart"></i> Voir Favoris';
    btn.classList.toggle('active');
    renderPhotos();
}

function renderPhotos() {
    const grid = document.getElementById('grid');
    grid.innerHTML = '';
    const data = onlyFavs ? photos.filter(p => p.fav) : photos;

    if(data.length === 0) {
        grid.innerHTML = '<p style="grid-column: 1/-1; text-align: center; color: #666; margin-top: 50px;">Aucune photo trouvée.</p>';
        return;
    }

    data.forEach(p => {
        const div = document.createElement('div');
        div.className = 'photo-card';
        
        // AJOUT DU BADGE LIGNE 4 CI-DESSOUS
        div.innerHTML = `
            <span class="photo-badge">Nichoir ${p.nichoir_id}</span>
            <img src="${p.url}" loading="lazy" onclick="openModal('${p.url}', '${p.nichoir_id}')" style="cursor: zoom-in;">
            <div class="photo-info">
                <span style="font-size: 0.8rem; color: #ccc;">${p.date}</span>
                <i id="heart-${p.id}" class="${p.fav ? 'fas fa-heart active' : 'far fa-heart'}" onclick="toggleFav(${p.id})"></i>
            </div>
        `;
        grid.appendChild(div);
    });
}

async function toggleFav(id) {
    try {
        const res = await fetch(`/api/fav/${id}`, {method: 'POST'});
        if (res.ok) {
            // Mise à jour visuelle immédiate
            const icon = document.getElementById(`heart-${id}`);
            if(icon) {
                // On inverse les classes
                if(icon.classList.contains('fas')) {
                    icon.className = 'far fa-heart'; // Devient vide
                } else {
                    icon.className = 'fas fa-heart active'; // Devient plein
                }
            }
            // Mise à jour des données locales pour que les filtres marchent
            const p = photos.find(x => x.id === id);
            if(p) p.fav = !p.fav;
        }
    } catch (e) {
        console.error("Erreur favoris", e);
    }
}

function openModal(url, nichoirId) {
    const modal = document.getElementById("photo-modal");
    const modalImg = document.getElementById("img-full");
    const captionText = document.getElementById("caption");
    
    modal.style.display = "block";
    modalImg.src = url;
    captionText.innerHTML = "Nichoir N°" + nichoirId; // Affiche le nom du nichoir
}

function closeModal() {
    document.getElementById("photo-modal").style.display = "none";
}

// Empêcher la fermeture si on clique sur l'image elle-même (optionnel)
document.getElementById('img-full').onclick = function(e) {
    e.stopPropagation();
}
