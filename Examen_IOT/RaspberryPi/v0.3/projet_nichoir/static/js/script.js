document.addEventListener('DOMContentLoaded', () => {
    // Vues
    const views = {
        dash: document.getElementById('view-dashboard'),
        gallery: document.getElementById('view-gallery'),
        config: document.getElementById('view-config')
    };

    // UI Elements
    const photoGrid = document.getElementById('photo-grid');
    const galleryTitle = document.getElementById('gallery-title');
    const links = document.querySelectorAll('.nav-links a');

    // Data
    const allPhotos = (typeof serverPhotos !== 'undefined') ? serverPhotos : [];
    let currentGalleryFilter = 'all'; // 'all', 'fav', or 'id'
    let currentSelectedId = null;

    // --- NAVIGATION ---
    function switchView(viewName) {
        Object.values(views).forEach(el => el.style.display = 'none');
        if (views[viewName]) views[viewName].style.display = 'block';
        
        // Active class sidebar
        links.forEach(l => l.classList.remove('active'));
        const activeLink = document.getElementById('nav-' + viewName);
        if(activeLink) activeLink.classList.add('active');
    }

    document.getElementById('nav-dash').onclick = () => switchView('dash');
    document.getElementById('nav-gallery').onclick = () => { renderGallery('all'); switchView('gallery'); };
    document.getElementById('nav-fav').onclick = () => { renderGallery('fav'); switchView('gallery'); };
    document.getElementById('nav-config').onclick = () => switchView('config');
    document.getElementById('view-gallery-btn').onclick = () => { 
        if(currentSelectedId) { renderGallery('id', currentSelectedId); switchView('gallery'); }
        else alert("Sélectionnez un nichoir");
    };

    // --- LOGIQUE BATTERIE ---
    function formatBattery(val) {
        let v = parseFloat(val);
        if (!v) return { text: "--", pct: 0 };
        
        // Si > 12, on suppose que c'est un pourcentage (ex: 100)
        if (v > 12) return { text: parseInt(v) + "%", pct: v };
        
        // Sinon c'est des volts (ex: 4.2)
        let pct = ((v - 3.3) / (4.2 - 3.3)) * 100;
        return { text: v + " V", pct: Math.max(0, Math.min(100, pct)) };
    }

    // --- SÉLECTION NICHOIR ---
    window.selectNichoir = (id, nom, batt, date, autonomie) => {
        currentSelectedId = id;
        document.getElementById('detail-id').innerText = nom;
        document.getElementById('detail-date').innerText = date || "Inconnu";
        document.getElementById('detail-auto').innerText = autonomie;
        
        const bInfo = formatBattery(batt);
        document.getElementById('detail-batt').innerText = bInfo.text;
        document.getElementById('detail-bar').style.width = bInfo.pct + "%";
        
        // Highlight
        document.querySelectorAll('.list-item').forEach(i => i.classList.remove('selected'));
        event.currentTarget.classList.add('selected');
    };

    // --- GALERIE & FAVORIS ---
    window.toggleFavorite = async (id, btn) => {
        event.stopPropagation(); // Empêche le clic sur la carte
        
        // Appel Serveur
        try {
            const res = await fetch(`/api/toggle_fav/${id}`, { method: 'POST' });
            if(res.ok) {
                // Mise à jour visuelle locale
                const photo = allPhotos.find(p => p.id == id);
                if(photo) photo.is_favorite = !photo.is_favorite;
                
                if (photo.is_favorite) btn.classList.add('active');
                else btn.classList.remove('active');
                
                // Si on est dans la vue favoris, on rafraichit pour enlever l'élément
                if(galleryTitle.innerText.includes("Favoris")) renderGallery('fav');
            }
        } catch(e) { console.error("Erreur API", e); }
    };

    function renderGallery(mode, filterId = null) {
        photoGrid.innerHTML = '';
        let list = allPhotos;
        
        if (mode === 'fav') {
            list = list.filter(p => p.is_favorite);
            galleryTitle.innerText = "❤️ Mes Photos Favorites";
        } else if (mode === 'id' && filterId) {
            list = list.filter(p => p.nichoir_id == filterId);
            galleryTitle.innerText = "Galerie - Nichoir " + filterId;
        } else {
            galleryTitle.innerText = "Toutes les Photos";
        }

        if (list.length === 0) {
            photoGrid.innerHTML = '<p class="empty-msg">Aucune photo trouvée.</p>';
            return;
        }

        list.forEach(p => {
            const div = document.createElement('div');
            div.className = 'photo-card';
            div.innerHTML = `
                <img src="${p.url}" loading="lazy">
                <div class="photo-info">
                    <span>${p.date}</span>
                    <button class="fav-btn ${p.is_favorite ? 'active' : ''}" onclick="toggleFavorite(${p.id}, this)">
                        <i class="fas fa-heart"></i>
                    </button>
                </div>
            `;
            photoGrid.appendChild(div);
        });
    }
});
