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
    // Afficher le panneau détail
    document.getElementById('detail-panel').style.display = 'block';
    
    // Remplir les infos
    document.getElementById('det-title').innerText = "Nichoir " + id;
    document.getElementById('det-auto').innerText = auto;
    document.getElementById('det-val').innerText = batt + "%";
    
    // Barre de progression
    let pct = parseFloat(batt);
    if(isNaN(pct)) pct = 0;
    document.getElementById('det-bar').style.width = pct + "%";

    // Style visuel sur la carte active
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
        div.innerHTML = `
            <img src="${p.url}" loading="lazy">
            <div class="photo-info">
                <span style="font-size: 0.8rem; color: #ccc;">${p.date}</span>
                <i class="${p.fav ? 'fas fa-heart active' : 'far fa-heart'}" onclick="toggleFav(${p.id})"></i>
            </div>
        `;
        grid.appendChild(div);
    });
}

async function toggleFav(id) {
    await fetch(`/api/fav/${id}`, {method: 'POST'});
    location.reload(); 
}
