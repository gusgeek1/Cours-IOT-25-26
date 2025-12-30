document.addEventListener('DOMContentLoaded', () => {
    // Récupération des éléments du DOM
    const dashView = document.getElementById('dashboard-view');
    const galleryView = document.getElementById('gallery-view');
    const photoGrid = document.getElementById('photo-grid');
    const galleryTitle = document.getElementById('gallery-title');
    
    // Boutons
    const navDash = document.getElementById('nav-dash');
    const navGallery = document.getElementById('nav-gallery');
    const btnViewSpecific = document.getElementById('view-gallery-btn');
    const btnBack = document.getElementById('back-btn');

    // Données (envoyées depuis Python)
    const allPhotos = (typeof serverPhotos !== 'undefined') ? serverPhotos : [];
    let currentSelectedId = null;

    // --- FONCTION : Sélectionner un Nichoir ---
    window.selectNichoir = (id, nom, batterie, date) => {
        currentSelectedId = id;
        
        // Mise à jour de l'affichage détail
        document.getElementById('detail-id').innerText = nom;
        document.getElementById('detail-date').innerText = date || "Jamais";
        document.getElementById('detail-batt').innerText = (batterie || "?") + " V";

        // Barre de progression (Simulation : 3V=0%, 4.2V=100%)
        let voltage = parseFloat(batterie) || 0;
        let percent = ((voltage - 3.0) / (4.2 - 3.0)) * 100;
        if (percent < 0) percent = 5;
        if (percent > 100) percent = 100;
        document.getElementById('detail-bar').style.width = percent + "%";

        // Style visuel de sélection
        document.querySelectorAll('.list-item').forEach(item => item.classList.remove('selected'));
        event.currentTarget.classList.add('selected');
    };

    // --- FONCTION : Afficher la Galerie ---
    function renderGallery(filterId = null) {
        photoGrid.innerHTML = ''; // On vide la grille
        let list = allPhotos;

        // Si un filtre est actif
        if (filterId) {
            list = allPhotos.filter(p => p.nichoir_id == filterId);
            galleryTitle.innerText = "Galerie - Nichoir " + filterId;
        } else {
            galleryTitle.innerText = "Toutes les photos";
        }

        if (list.length === 0) {
            photoGrid.innerHTML = '<p style="color:#aaa; grid-column:1/-1; text-align:center;">Aucune photo trouvée.</p>';
            return;
        }

        // Création des cartes photo
        list.forEach(photo => {
            const card = document.createElement('div');
            card.className = 'photo-card';
            card.innerHTML = `
                <img src="${photo.url}" alt="Capture" loading="lazy">
                <div class="photo-info">
                    <span>${photo.date}</span>
                    <span style="font-size:0.8em; opacity:0.7;">${photo.battery}V</span>
                </div>
            `;
            photoGrid.appendChild(card);
        });
    }

    // --- Événements ---

    // Bouton "Voir Photos de ce nichoir"
    btnViewSpecific.addEventListener('click', () => {
        if (!currentSelectedId) {
            alert("Veuillez d'abord cliquer sur un nichoir dans la liste.");
            return;
        }
        dashView.style.display = 'none';
        galleryView.style.display = 'block';
        renderGallery(currentSelectedId);
    });

    // Bouton Retour
    btnBack.addEventListener('click', () => {
        galleryView.style.display = 'none';
        dashView.style.display = 'grid';
    });

    // Menu Galerie
    navGallery.addEventListener('click', (e) => {
        e.preventDefault();
        dashView.style.display = 'none';
        galleryView.style.display = 'block';
        renderGallery(null); // Affiche tout
        navDash.classList.remove('active');
        navGallery.classList.add('active');
    });

    // Menu Dashboard
    navDash.addEventListener('click', (e) => {
        e.preventDefault();
        galleryView.style.display = 'none';
        dashView.style.display = 'grid';
        navGallery.classList.remove('active');
        navDash.classList.add('active');
    });
});
