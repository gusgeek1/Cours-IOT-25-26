let showOnlyFavs = false;

function showView(id) {
    document.querySelectorAll('.view').forEach(v => v.style.display = 'none');
    document.getElementById(id).style.display = 'block';
    
    document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));
    // On ne gère plus les favoris en haut à gauche
    if(id === 'gallery') renderPhotos();
}

function selectNichoir(id, batt, auto) {
    document.getElementById('det-id').innerText = "NCH-" + id;
    document.getElementById('det-auto').innerText = auto;
    // On gère l'affichage batterie 100V vs % [cite: 84, 85]
    let pct = parseFloat(batt) > 12 ? batt : ((batt - 3.3) / 0.9) * 100;
    document.getElementById('det-bar').style.width = Math.max(5, Math.min(100, pct)) + "%";
}

function toggleFavFilter() {
    showOnlyFavs = !showOnlyFavs;
    document.getElementById('fav-toggle').innerText = showOnlyFavs ? "Voir Tout" : "Voir Favoris";
    renderPhotos();
}

function renderPhotos() {
    const grid = document.getElementById('grid');
    grid.innerHTML = '';
    const filtered = showOnlyFavs ? photos.filter(p => p.fav) : photos;
    
    if(filtered.length === 0) {
        grid.innerHTML = '<p style="grid-column: 1/-1; text-align: center; color: var(--text-muted);">Aucune photo.</p>';
        return;
    }

    filtered.forEach(p => {
        const card = document.createElement('div');
        card.className = 'photo-card';
        card.innerHTML = `
            <img src="${p.url}" onerror="this.src='https://via.placeholder.com/300?text=Erreur+Image'">
            <div class="photo-info">
                <span>${p.date}</span>
                <i class="fa-heart ${p.fav?'fas active':'far'}" onclick="toggleFav(${p.id})"></i>
            </div>
        `;
        grid.appendChild(card);
    });
}

async function toggleFav(id) {
    await fetch(`/api/fav/${id}`, {method: 'POST'});
    location.reload(); // Rechargement simple pour rafraîchir les données de la session
}
