// === ÂM THANH ===
const sfx = { draw: new Audio('rut-bai.mp3'), summon: new Audio('trieu-hoi.mp3'), attack: new Audio('tan-cong.mp3'), damage: new Audio('mat-mau.mp3'), phase: new Audio('chuyen-luot.mp3'), victory: new Audio('chienthang.mp3'), defeat: new Audio('thatbai.mp3') };
function playSound(s) { s.currentTime = 0; s.play().catch(() => { }); }

// For Firebase JS SDK v7.20.0 and later, measurementId is optional
const firebaseConfig = {
    apiKey: "AIzaSyBxtJY9H3lLjS0UiDlqjW-McDKuqbS5Vkc",
    authDomain: "tamquocyugioh.firebaseapp.com",
    projectId: "tamquocyugioh",
    storageBucket: "tamquocyugioh.firebasestorage.app",
    messagingSenderId: "647925030468",
    appId: "1:647925030468:web:fc28f9183346bb65c8a611",
    measurementId: "G-XETB1XG2PE"
};

// Khởi tạo Firebase nếu có config
let db = null;
if (firebaseConfig.apiKey) {
    if (!firebase.apps.length) {
        firebase.initializeApp(firebaseConfig);
    }
    db = firebase.firestore();
}

let currentUserStatus = null; // 'guest' or Firebase User Object
let uid = null;

// === STATE & PROGRESSION ===
let gameMode = 'ai';
let aiDifficulty = 'medium';
let currentPlayer = 1;
let pvpBuildPhase = 0;
let p1Deck = [], p2Deck = [];
const MAX_LP = 8000, DECK_SIZE = 30;
let selectedPlayerDeck = [];
let isGameOver = false;
let pendingSummon = { slotIndex: null, handIndex: null, card: null, row: null };
let tributeState = { needed: 0, selected: [], callback: null };
let turnTimerId = null, turnTimeLeft = 30;

let playerGold = 0;
let unlockedCards = [];
let gachaPity = 0;
let completedStories = [];
let playerQuests = [
    { id: 'win_ai', name: 'Thắng Đấu AI 1 lần', reward: 50, require: 1, current: 0, done: false },
    { id: 'summon_10', name: 'Triệu hồi 10 thẻ', reward: 30, require: 10, current: 0, done: false },
    { id: 'damage_5000', name: 'Gây 5000 sát thương', reward: 40, require: 5000, current: 0, done: false },
    { id: 'destroy_5', name: 'Tiêu diệt 5 thẻ địch', reward: 30, require: 5, current: 0, done: false }
];

// === SYSTEM SAVING & PROGRESSION ===
function initProgression() {
    // Tải dữ liệu từ LocalStorage (phiên bản không dùng Firebase/Dự phòng)
    const savedGold = localStorage.getItem('tamquoc_gold');
    if (savedGold !== null) playerGold = parseInt(savedGold);
    
    const savedCards = localStorage.getItem('tamquoc_unlocked');
    if (savedCards) unlockedCards = JSON.parse(savedCards);
    
    const savedPity = localStorage.getItem('tamquoc_gachapity');
    if (savedPity !== null) gachaPity = parseInt(savedPity);
    
    const savedStories = localStorage.getItem('tamquoc_stories');
    if (savedStories) completedStories = JSON.parse(savedStories);
    
    const savedQuests = localStorage.getItem('tamquoc_quests');
    if (savedQuests) playerQuests = JSON.parse(savedQuests);
    
    updateGoldUI();
}

function saveProgress() {
    updateGoldUI();
    saveToCloud(); // Gọi hàm lưu chung
}

function saveQuests() {
    saveToCloud(); // Quests cũng đưa vào lưu chung
}

function saveToCloud() {
    // Nếu là khách, KHÔNG LƯU VĨNH VIỄN
    if (currentUserStatus === 'guest') return;

    // Nếu là user thực tế có Firebase DB
    if (currentUserStatus && uid && db) {
        db.collection("users").doc(uid).set({
            playerGold: playerGold,
            unlockedCards: unlockedCards,
            gachaPity: gachaPity,
            completedStories: completedStories,
            playerQuests: playerQuests
        }, { merge: true }).catch(err => {
            console.error("Lỗi khi lưu dữ liệu lên Cloud:", err);
        });
    } else {
        // Lưu local cho tài khoản khách
        localStorage.setItem('tamquoc_gold', playerGold);
        localStorage.setItem('tamquoc_unlocked', JSON.stringify(unlockedCards));
        localStorage.setItem('tamquoc_gachapity', gachaPity);
        localStorage.setItem('tamquoc_stories', JSON.stringify(completedStories));
        localStorage.setItem('tamquoc_quests', JSON.stringify(playerQuests));
    }
}

// Khởi tạo ngẫu nhiên thẻ cho tài khoản mới hoặc khách
function initRandomCardsAndGold() {
    let allIds = cardDb.map(c => c.id);
    for (let i = allIds.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [allIds[i], allIds[j]] = [allIds[j], allIds[i]];
    }
    unlockedCards = allIds.slice(0, 30);
    playerGold = 1000000;
    gachaPity = 0;
    completedStories = [];
    saveProgress();
}

function loadCloudData(data) {
    playerGold = data.playerGold !== undefined ? data.playerGold : 1000000;
    unlockedCards = data.unlockedCards || [];
    gachaPity = data.gachaPity || 0;
    completedStories = data.completedStories || [];
    if (data.playerQuests) {
        playerQuests = data.playerQuests;
    }
    updateGoldUI();
    if (unlockedCards.length === 0) {
        initRandomCardsAndGold();
    }
}

// Tải dữ liệu của User từ Firestore
function fetchUserDataFromFirebase(user) {
    if (!db) {
        showToast("Lỗi: Firebase chưa được config!");
        return;
    }
    const userRef = db.collection("users").doc(user.uid);
    userRef.get().then(doc => {
        if (doc.exists) {
            loadCloudData(doc.data());
        } else {
            // User mới chơi lần đầu, tạo data mẫu
            initRandomCardsAndGold();
        }
        transitionToModeSelection(); // Xong thì vào game
    }).catch(err => {
        console.error("Lỗi khi tải data:", err);
        showToast("Lỗi tải dữ liệu. Bạn đang tiếp tục bằng tạo mới.");
        initRandomCardsAndGold();
        transitionToModeSelection();
    });
}

function transitionToModeSelection() {
    const loginScreen = document.getElementById('login-screen');
    if (loginScreen) {
        loginScreen.style.opacity = '0';
        setTimeout(() => { loginScreen.style.display = 'none'; }, 300);
    }
    const modeScreen = document.getElementById('mode-selection-screen');
    if (modeScreen) {
        modeScreen.style.display = 'flex';
    }
    playSound(sfx.draw);
}

// Lắng nghe trạng thái đăng nhập tự động
if (firebaseConfig.apiKey) {
    firebase.auth().onAuthStateChanged((user) => {
        if (user) {
            // Đã đăng nhập trước đó hoặc vừa đăng nhập thành công
            currentUserStatus = user;
            uid = user.uid;
            document.getElementById('auth-email-modal').style.display = 'none';
            showToast(`Chào mừng trở lại!`);
            fetchUserDataFromFirebase(user);
        }
    });
}

// === LOGIN ACTION MAPPER ===
function loginAction(method) {
    if (method === 'Khách') {
        currentUserStatus = 'guest';
        uid = null;
        showToast('Đã đăng nhập bằng tài khoản Khách');
        initRandomCardsAndGold();
        transitionToModeSelection();
        return;
    }

    if (!firebaseConfig.apiKey) {
        showToast("BẠN CHƯA CÀI FIREBASE CONFIG!");
        return;
    }

    if (method === 'Tạo tài khoản') {
        document.getElementById('auth-email-modal').style.display = 'flex';
    } else if (method === 'Gmail') {
        const provider = new firebase.auth.GoogleAuthProvider();
        firebase.auth().signInWithPopup(provider).catch(err => {
            showToast("Lỗi đăng nhập Gmail: " + err.message);
        });
    }
}

// Xử lý Popup Email
function closeAuthEmail() {
    document.getElementById('auth-email-modal').style.display = 'none';
    document.getElementById('auth-error-msg').innerText = '';
}

function getEmailPass() {
    const e = document.getElementById('auth-email').value;
    const p = document.getElementById('auth-password').value;
    return { e, p };
}

function submitEmailLogin() {
    const { e, p } = getEmailPass();
    if (!e || !p) return;
    firebase.auth().signInWithEmailAndPassword(e, p).catch(err => {
        document.getElementById('auth-error-msg').innerText = err.message;
    });
}

function submitEmailRegister() {
    const { e, p } = getEmailPass();
    if (!e || !p) return;
    firebase.auth().createUserWithEmailAndPassword(e, p).catch(err => {
        document.getElementById('auth-error-msg').innerText = err.message;
    });
}


function updateGoldUI() {
    const goldMenu = document.getElementById('menu-gold-amount');
    const goldShop = document.getElementById('shop-gold-amount');
    if (goldMenu) goldMenu.innerText = playerGold;
    if (goldShop) goldShop.innerText = playerGold;
}

function openShop() {
    document.getElementById('shop-modal').style.display = 'flex';
    updateGoldUI();
}
function closeShop() {
    document.getElementById('shop-modal').style.display = 'none';
}

function openGuide() { document.getElementById('guide-modal').style.display = 'flex'; }
function closeGuide() { document.getElementById('guide-modal').style.display = 'none'; }

// Hàm saveQuests đã được đẩy lên trên cùng khối save.
// Chúng ta xoá ở đây để tránh trùng lặp.

function openQuest() {
    document.getElementById('quest-modal').style.display = 'flex';
    renderQuests();
}
function closeQuest() {
    document.getElementById('quest-modal').style.display = 'none';
}

function addQuestProgress(qId, amount) {
    let q = playerQuests.find(x => x.id === qId);
    if (q && !q.done) {
        q.current += amount;
        if (q.current >= q.require) q.current = q.require;
        saveQuests();
    }
}

function claimQuest(qId) {
    let q = playerQuests.find(x => x.id === qId);
    if (q && q.current >= q.require && !q.done) {
        q.done = true;
        playerGold += q.reward;
        // Reset để làm tiếp (nếu muốn cày vô hạn)
        q.current = 0;
        q.done = false;

        saveProgress();
        saveQuests();
        showToast(`Nhận ${q.reward} Vàng thành công!`);
        renderQuests();
    }
}

function renderQuests() {
    const container = document.getElementById('quest-list-container');
    if (!container) return;
    container.innerHTML = '';
    playerQuests.forEach(q => {
        const pct = Math.min(100, (q.current / q.require) * 100);
        const canClaim = q.current >= q.require && !q.done;

        let html = `
            <div class="quest-item ${canClaim ? 'completed' : ''}">
                <div class="quest-info">
                    <div class="quest-name">${q.name}</div>
                    <div class="quest-reward">Thưởng: ${q.reward} 🪙</div>
                    <div class="quest-progress-bg">
                        <div class="quest-progress-fill" style="width: ${pct}%"></div>
                    </div>
                    <div class="quest-progress-text">${q.current} / ${q.require}</div>
                </div>
                <button class="btn btn-claim" ${!canClaim ? 'disabled' : ''} onclick="claimQuest('${q.id}')">
                    ${canClaim ? 'NHẬN' : 'CHƯA ĐẠT'}
                </button>
            </div>
        `;
        container.innerHTML += html;
    });
}

function buyGachaPack(packType) {
    let cost = 0;
    let prob3 = 0, prob4 = 0, prob5 = 0;

    // Gói x10: Giá trị * 10
    if (packType === 'bronze') {
        cost = 500; prob3 = 15; prob4 = 4; prob5 = 1;
    } else if (packType === 'silver') {
        cost = 1000; prob3 = 30; prob4 = 15; prob5 = 5;
    } else if (packType === 'gold') {
        cost = 2000; prob3 = 40; prob4 = 30; prob5 = 10;
    } else return;

    if (playerGold < cost) { showToast(`Không đủ Vàng! Cần ${cost} Vàng.`); return; }
    playerGold -= cost;
    saveProgress();

    let drawnCards = [];
    for (let i = 0; i < 10; i++) {
        let isPityHit = false;
        if (packType === 'gold') {
            gachaPity++;
            if (gachaPity >= 50) {
                isPityHit = true;
                gachaPity = 0;
            }
            localStorage.setItem('tamquoc_gachapity', gachaPity);
        }

        const rand = Math.random() * 100;
        let targetStars = 0;
        if (isPityHit) {
            targetStars = 5;
        } else {
            if (rand < prob5) targetStars = 5;
            else if (rand < prob5 + prob4) targetStars = 4;
            else if (rand < prob5 + prob4 + prob3) targetStars = 3;
            else targetStars = Math.floor(Math.random() * 2) + 1;
        }

        if (targetStars === 5 && packType === 'gold') {
            gachaPity = 0; // Reset pity
            localStorage.setItem('tamquoc_gachapity', gachaPity);
        }

        let possibleCards = cardDb.filter(c => c.stars === targetStars);
        if (possibleCards.length === 0) possibleCards = cardDb; // Dự phòng
        const card = possibleCards[Math.floor(Math.random() * possibleCards.length)];
        drawnCards.push(card);
    }

    showGachaAnimation(drawnCards, cost);
}

function showGachaAnimation(cards, cost) {
    document.getElementById('shop-modal').style.display = 'none';
    const overlay = document.getElementById('gacha-animation-overlay');
    overlay.style.display = 'flex';

    const pack = document.getElementById('gacha-pack');
    const result = document.getElementById('gacha-result');
    const resultGrid = document.getElementById('gacha-result-grid');
    const resultMsg = document.getElementById('gacha-result-msg');

    pack.style.display = 'block';
    result.style.display = 'none';
    pack.classList.add('anim-shake-pack');
    playSound(sfx.draw);

    setTimeout(() => {
        pack.classList.remove('anim-shake-pack');
        pack.style.display = 'none';
        playSound(sfx.summon);

        result.style.display = 'flex';
        resultGrid.innerHTML = '';

        let newCardsCount = 0;
        let refundTotal = 0;

        cards.forEach((card, index) => {
            const isDuplicate = unlockedCards.includes(card.id);
            if (!isDuplicate) {
                unlockedCards.push(card.id);
                newCardsCount++;
            } else {
                refundTotal += Math.floor((cost / 10) * 0.4);
            }

            const cardDiv = document.createElement('div');
            cardDiv.className = 'gacha-result-card ' + getStarColorClass(card.stars);
            if (isDuplicate) cardDiv.classList.add('duplicate');
            cardDiv.style.backgroundImage = `url('${card.img || CARD_BACK}')`;
            cardDiv.style.animation = `modalSlideIn 0.3s ease backwards`;
            cardDiv.style.animationDelay = `${index * 0.1}s`;
            resultGrid.appendChild(cardDiv);
        });

        if (refundTotal > 0) {
            playerGold += refundTotal;
        }
        saveProgress();

        let msg = `<span style="color:#2ecc71">✨ Rút được ${newCardsCount} thẻ mới! ✨</span>`;
        if (refundTotal > 0) {
            msg += `<br><span style="color:#f1c40f">Hoàn trả thẻ trùng: +${refundTotal} 🪙</span>`;
        }
        resultMsg.innerHTML = msg;
    }, 1500);
}

function closeGacha() {
    document.getElementById('gacha-animation-overlay').style.display = 'none';
    openShop();
}

let game = {
    playerLp: MAX_LP, enemyLp: MAX_LP,
    turn: 'player', phase: 'standby',
    playerDeck: [], enemyDeck: [],
    playerHand: [], enemyHand: [],
    playerAtkField: [null, null, null, null, null],
    playerDefField: [null, null, null, null, null],
    enemyAtkField: [null, null, null, null, null],
    enemyDefField: [null, null, null, null, null],
    selectedCardIndex: null, selectedCardRow: null,
    selectedHandIndex: null,
    isAnimating: false
};

function checkSelectorAndProceed() {
    if (localStorage.getItem('tamquoc_free5star_claimed') === 'true') {
        pvpBuildPhase = 1;
        goToDeckBuilderFromMode();
    } else {
        openSelectorModal();
    }
}

function openSelectorModal() {
    const overlay = document.getElementById('selector-modal');
    const grid = document.getElementById('selector-grid');
    grid.innerHTML = '';

    let fiveStars = cardDb.filter(c => c.stars === 5).slice(0, 4);

    fiveStars.forEach(c => {
        let cardDiv = document.createElement('div');
        cardDiv.className = 'selector-card-wrapper';
        cardDiv.innerHTML = `
            <div class="gacha-result-card stars-5" style="background-image: url('${c.img || CARD_BACK}'); box-shadow: none; border: 2px solid #f1c40f;"></div>
            <div class="db-card-name-below">${c.name}</div>
        `;
        cardDiv.onclick = () => claimFree5Star(c.id);
        grid.appendChild(cardDiv);
    });

    overlay.style.display = 'flex';
}

function claimFree5Star(cardId) {
    if (!unlockedCards.includes(cardId)) {
        unlockedCards.push(cardId);
        saveProgress();
    }
    localStorage.setItem('tamquoc_free5star_claimed', 'true');
    document.getElementById('selector-modal').style.display = 'none';
    showToast('Nhận chủ lực 5 Sao thành công!');

    pvpBuildPhase = 1;
    goToDeckBuilderFromMode();
}

function closeSelector() {
    document.getElementById('selector-modal').style.display = 'none';
    pvpBuildPhase = 1;
    goToDeckBuilderFromMode();
}

// === MODE SELECTION ===
let activeStoryId = null;

function openStoryModal() {
    document.getElementById('story-modal').style.display = 'flex';
    renderStories();
}

function closeStoryModal() {
    document.getElementById('story-modal').style.display = 'none';
}

function renderStories() {
    const container = document.getElementById('story-list-container');
    container.innerHTML = '';
    storyData.forEach(story => {
        const isCompleted = completedStories.includes(story.id);
        const rewardCardsHtml = story.rewardCards.slice(0, 4).map(id => {
            const card = cardDb.find(c => c.id === id);
            return card ? `<div class="reward-preview-card" style="background-image:url('${card.img}')" title="${card.name}"></div>` : '';
        }).join('');
        
        container.innerHTML += `
            <div class="story-item ${isCompleted ? 'completed' : ''}">
                <div class="story-item-info">
                    <div class="story-item-title">${story.title}</div>
                    <div class="story-enemy-label">⚔️ Địch: ${story.enemyName}</div>
                    <div class="story-item-desc">${story.description}</div>
                    <div class="reward-preview">
                        <span class="reward-preview-label">Thưởng:</span>
                        <div class="reward-preview-cards">${rewardCardsHtml}</div>
                        <span style="color:#f1c40f; font-size:12px; font-weight:bold; margin-left:10px;">+${story.goldReward} 🪙</span>
                    </div>
                </div>
                <button class="btn-play-story" onclick="playStory('${story.id}')">CHIẾN NGAY</button>
            </div>
        `;
    });
}

function playStory(storyId) {
    document.getElementById('story-modal').style.display = 'none';
    const ms = document.getElementById('mode-selection-screen');
    if (ms) ms.style.display = 'none';
    gameMode = 'story';
    activeStoryId = storyId;
    initGame();
}

function closeStoryReward() {
    document.getElementById('story-reward-modal').style.display = 'none';
    goToModeSelect();
    openStoryModal();
}

function selectMode(mode) {
    gameMode = mode;
    document.querySelectorAll('.mode-card').forEach(c => c.classList.remove('active'));
    document.getElementById(mode === 'ai' ? 'mc-ai' : 'mc-pvp').classList.add('active');
    const diffPanel = document.getElementById('ai-difficulty-panel');
    if (mode === 'ai') { diffPanel.style.display = 'block'; }
    else {
        diffPanel.style.display = 'none';
        checkSelectorAndProceed();
    }
}
function selectDifficulty(diff) {
    aiDifficulty = diff;
    goToDeckBuilderFromMode();
}
function goToDeckBuilderFromMode() {
    document.getElementById('mode-selection-screen').style.display = 'none';
    selectedPlayerDeck = [];
    if (gameMode === 'pvp') {
        pvpBuildPhase = 1;
        document.getElementById('deck-builder-title').innerText = '⚔️ NGƯỜI CHƠI 1 - CHỌN BÀI ⚔️';
    } else if (gameMode === 'story') {
        const s = storyData.find(x => x.id === activeStoryId);
        document.getElementById('deck-builder-title').innerText = `📜 KHAI BÀI: ${s.title.toUpperCase()} 📜`;
    } else {
        document.getElementById('deck-builder-title').innerText = '⚔️ XÂY DỰNG BỘ BÀI ⚔️';
    }
    showDeckBuilder();
}
function goToModeSelect() {
    document.getElementById('game-over-modal').style.display = 'none';
    Object.values(sfx).forEach(s => { s.pause(); s.currentTime = 0; });
    document.getElementById('mode-selection-screen').style.display = 'flex';
    document.getElementById('ai-difficulty-panel').style.display = 'none';
    document.querySelectorAll('.mode-card').forEach(c => c.classList.remove('active'));
}

function backFromDeckBuilder() {
    document.getElementById('deck-builder-screen').style.display = 'none';
    goToModeSelect();
}

function surrenderGame() {
    if (confirm("Bạn có chắc chắn muốn thoát game ra Menu chính không?")) {
        isGameOver = true;
        clearTurnTimer();
        game.isAnimating = false; // Ngăn chặn các sự kiện delay
        goToModeSelect();
    }
}

// === TIMER ===
function startTurnTimer() { clearTurnTimer(); turnTimeLeft = 30; updateTimerDisplay(); turnTimerId = setInterval(() => { turnTimeLeft--; updateTimerDisplay(); if (turnTimeLeft <= 0) { clearTurnTimer(); autoAdvancePhase(); } }, 1000); }
function clearTurnTimer() { if (turnTimerId) { clearInterval(turnTimerId); turnTimerId = null; } }
function updateTimerDisplay() { const el = document.getElementById('turn-timer'); if (el) { el.innerText = turnTimeLeft + 's'; el.style.color = turnTimeLeft <= 10 ? '#e74c3c' : '#f1c40f'; } }
function autoAdvancePhase() { if (isGameOver || game.isAnimating) return; if (game.turn === 'player') { game.selectedHandIndex = null; game.selectedCardIndex = null; renderHand(); renderField(); nextPhase(); } }

// === UI ELEMENTS ===
let ui = {};
function initUI() {
    ui = {
        phaseText: document.getElementById('phase-indicator'),
        btnNextPhase: document.getElementById('btn-next-phase'),
        handArea: document.getElementById('hand-area'),
        enemyHandArea: document.getElementById('enemy-hand-area'),
        pAtkZone: document.getElementById('player-atk-zone').children,
        pDefZone: document.getElementById('player-def-zone').children,
        eAtkZone: document.getElementById('enemy-atk-zone').children,
        eDefZone: document.getElementById('enemy-def-zone').children,
        cardImg: document.getElementById('card-preview-img'),
        cardName: document.getElementById('desc-name'),
        cardAtk: document.getElementById('desc-atk'),
        cardDef: document.getElementById('desc-def'),
        cardDesc: document.getElementById('desc-text'),
        animOverlay: document.getElementById('animation-overlay')
    };
}

// === STARS & TYPE DISPLAY ===
function getStarsDisplay(s) { return '★'.repeat(s || 0) + '☆'.repeat(5 - (s || 0)); }
function getStarColorClass(s) { return s >= 5 ? 'stars-5' : s >= 4 ? 'stars-4' : s >= 3 ? 'stars-3' : s >= 2 ? 'stars-2' : 'stars-1'; }
function getTypeLabel(t) { return t === 'vo' ? '⚔️ Võ Tướng' : t === 'van' ? '📜 Văn Tướng' : '📜 Khác'; }

function showCardPreview(card) {
    if (!card) return;
    ui.cardImg.src = card.faceDown ? CARD_BACK : (card.img || CARD_BACK);
    ui.cardName.innerText = card.faceDown ? 'Tướng Địch' : (card.name || '?');
    ui.cardAtk.innerText = card.faceDown ? '-' : (card.atk || 0);
    ui.cardDef.innerText = card.faceDown ? '-' : (card.def || 0);
    ui.cardDesc.innerText = card.faceDown ? 'Bài của kẻ địch.' : (card.desc || '');
    const starsEl = document.getElementById('desc-stars');
    if (starsEl) { starsEl.innerText = card.faceDown ? '☆☆☆☆☆' : getStarsDisplay(card.stars); starsEl.className = 'card-stars ' + (card.faceDown ? '' : getStarColorClass(card.stars)); }
    const typeEl = document.getElementById('desc-type');
    if (typeEl) { if (card.faceDown || !card.type) { typeEl.innerText = ''; typeEl.className = 'card-type-badge'; } else { typeEl.innerText = getTypeLabel(card.type); typeEl.className = 'card-type-badge type-' + card.type; } }
}

// === RENDER ===
function renderEnemyHand() {
    ui.enemyHandArea.innerHTML = '';
    game.enemyHand.forEach(card => { if (!card) return; let d = document.createElement('div'); d.className = 'card-in-hand enemy-card-in-hand'; d.style.backgroundImage = `url('${CARD_BACK}')`; ui.enemyHandArea.appendChild(d); });
}
function renderHand() {
    ui.handArea.innerHTML = '';
    game.playerHand.forEach((card, i) => {
        if (!card) return; let d = document.createElement('div'); d.className = 'card-in-hand';
        d.style.backgroundImage = `url('${card.img || CARD_BACK}')`;
        if (game.selectedHandIndex === i) d.classList.add('hand-card-selected');
        d.onmouseenter = () => showCardPreview(card);
        d.onclick = () => { if (isGameOver) return; if (game.phase === 'your_main1' || game.phase === 'your_main2') { game.selectedHandIndex = (game.selectedHandIndex === i) ? null : i; renderHand(); renderField(); } else showToast('Chỉ triệu hồi trong Main Phase!'); };
        ui.handArea.appendChild(d);
    });
}

function renderFieldRow(zoneEls, fieldArr, side, rowType) {
    const isTributing = tributeState.needed > 0 && side === 'player';
    for (let i = 0; i < 5; i++) {
        zoneEls[i].innerHTML = ''; zoneEls[i].classList.remove('slot-available', 'tribute-selectable', 'tribute-selected'); zoneEls[i].onclick = null;
        let card = fieldArr[i];
        if (card) {
            let d = document.createElement('div');
            let cls = 'card on-field';
            if (card.faceDown) cls += ' face-down';
            cls += rowType === 'def' ? ' in-def-row' : ' in-atk-row';
            d.className = cls;
            d.style.backgroundImage = `url('${card.faceDown ? CARD_BACK : (card.img || CARD_BACK)}')`;
            d.onmouseenter = () => showCardPreview(card);
            if (isTributing) {
                // Tribute mode: click to select/deselect tribute
                const isSelected = tributeState.selected.some(t => t.row === rowType && t.idx === i);
                if (isSelected) { zoneEls[i].classList.add('tribute-selected'); d.style.boxShadow = '0 0 20px #e74c3c'; d.style.border = '3px solid #e74c3c'; }
                else { zoneEls[i].classList.add('tribute-selectable'); }
                const idx = i, rt = rowType;
                d.onclick = () => handleTributeClick(idx, rt);
            } else if (side === 'player' && game.selectedHandIndex !== null && (game.phase === 'your_main1' || game.phase === 'your_main2')) {
                // Determine if we want to use this occupied slot for a tribute summon
                const cardInHand = game.playerHand[game.selectedHandIndex];
                if (cardInHand && getTributeCount(cardInHand.stars) > 0) {
                    zoneEls[i].classList.add('tribute-selectable');
                    const slot = i, rt = rowType;
                    d.onclick = () => summonCardToSlot(slot, rt, true);
                } else {
                    d.onclick = () => handleFieldClick(i, side, rowType);
                }
            } else {
                d.onclick = () => handleFieldClick(i, side, rowType);
            }
            zoneEls[i].appendChild(d);
        } else if (side === 'player' && !isTributing && game.selectedHandIndex !== null && (game.phase === 'your_main1' || game.phase === 'your_main2')) {
            zoneEls[i].classList.add('slot-available');
            const slot = i, rt = rowType;
            zoneEls[i].onclick = () => summonCardToSlot(slot, rt, false);
        }
    }
}
function renderField() {
    renderFieldRow(ui.eAtkZone, game.enemyAtkField, 'enemy', 'atk');
    renderFieldRow(ui.eDefZone, game.enemyDefField, 'enemy', 'def');
    renderFieldRow(ui.pDefZone, game.playerDefField, 'player', 'def');
    renderFieldRow(ui.pAtkZone, game.playerAtkField, 'player', 'atk');
}

function updateLP() {
    document.getElementById('ui-player-lp').innerText = Math.max(0, game.playerLp);
    document.getElementById('ui-enemy-lp').innerText = Math.max(0, game.enemyLp);
    updateHPBars();
}
function updateDeckCounts() {
    const pc = document.getElementById('player-deck-count'), ec = document.getElementById('enemy-deck-count');
    if (pc) pc.innerText = game.playerDeck.length; if (ec) ec.innerText = game.enemyDeck.length;
}
function rowCount(f) { return f.filter(c => c !== null).length; }
function totalEnemyField() { return rowCount(game.enemyAtkField) + rowCount(game.enemyDefField); }
function totalPlayerField() { return rowCount(game.playerAtkField) + rowCount(game.playerDefField); }

// === HP BAR ===
function updateHPBars() {
    const pp = Math.max(0, game.playerLp / MAX_LP * 100), ep = Math.max(0, game.enemyLp / MAX_LP * 100);
    const pb = document.getElementById('player-hp-bar'), eb = document.getElementById('enemy-hp-bar');
    if (pb) { pb.style.width = pp + '%'; pb.style.background = pp > 60 ? 'linear-gradient(90deg,#2ecc71,#27ae60)' : pp > 30 ? 'linear-gradient(90deg,#f39c12,#e67e22)' : 'linear-gradient(90deg,#e74c3c,#c0392b)'; }
    if (eb) { eb.style.width = ep + '%'; eb.style.background = ep > 60 ? 'linear-gradient(90deg,#2ecc71,#27ae60)' : ep > 30 ? 'linear-gradient(90deg,#f39c12,#e67e22)' : 'linear-gradient(90deg,#e74c3c,#c0392b)'; }
}

// === DAMAGE & TOAST ===
function showDamageNumber(target, amount) {
    const lpEl = document.getElementById(target === 'player' ? 'ui-player-lp' : 'ui-enemy-lp'); if (!lpEl) return;
    const r = lpEl.getBoundingClientRect(), d = document.createElement('div');
    d.className = 'damage-number ' + (target === 'player' ? 'dmg-player' : 'dmg-enemy'); d.innerText = '-' + amount;
    d.style.left = (r.left + r.width / 2 - 30) + 'px'; d.style.top = (r.bottom + 5) + 'px';
    document.body.appendChild(d); setTimeout(() => { if (document.body.contains(d)) document.body.removeChild(d); }, 2000);
}
function showToast(msg) { const t = document.createElement('div'); t.className = 'position-toast'; t.innerText = msg; document.body.appendChild(t); setTimeout(() => { if (document.body.contains(t)) document.body.removeChild(t); }, 1200); }

// === DRAW ===
function animateCardDraw(isPlayer, cardData, callback) {
    if (isGameOver || !cardData) return;
    game.isAnimating = true; playSound(sfx.draw);
    const ac = document.createElement('div'); ac.className = `card animating-card ${isPlayer ? 'anim-player-draw' : 'anim-enemy-draw'}`;
    ac.style.backgroundImage = `url('${isPlayer ? (cardData.img || CARD_BACK) : CARD_BACK}')`;
    ui.animOverlay.appendChild(ac); updateDeckCounts();
    setTimeout(() => {
        if (ui.animOverlay.contains(ac)) ui.animOverlay.removeChild(ac);
        if (isPlayer) { game.playerHand.push(cardData); renderHand(); }
        else { game.enemyHand.push(cardData); renderEnemyHand(); }
        game.isAnimating = false;
        if (callback) callback();
    }, 600);
}

function drawInitialHands() {
    if (isGameOver) return;
    game.isAnimating = true; clearTurnTimer();
    let pCards = [], eCards = [];
    for (let i = 0; i < 5; i++) {
        if (game.playerDeck.length > 0) pCards.push(game.playerDeck.pop());
        if (game.enemyDeck.length > 0) {
            const ec = game.enemyDeck.pop(); ec.faceDown = true;
            eCards.push(ec);
        }
    }

    let count = 0;
    const interval = setInterval(() => {
        if (count < pCards.length) animateCardDraw(true, pCards[count]);
        if (count < eCards.length) animateCardDraw(false, eCards[count]);
        count++;
        if (count >= 5) {
            clearInterval(interval);
            setTimeout(() => {
                game.isAnimating = false;
                setPhase('your_main1', 'GIAI ĐOẠN CHÍNH 1');
            }, 800);
        }
    }, 400);
}

function playerManualDraw() {
    if (isGameOver) return;
    if (game.turn !== 'player') {
        showToast('Chỉ được rút bài trong lượt của mình!');
        return;
    }
    if (game.phase === 'standby' || game.phase === 'dealing') {
        showToast('Hãy chờ hết giai đoạn chuẩn bị!');
        return;
    }
    if (game.playerDeck.length === 0) {
        showToast('Hết bài để rút!');
        return;
    }
    const c = game.playerDeck.pop();
    animateCardDraw(true, c);
}

// === SUMMON (hệ thống hiến tế) ===
function getTributeCount(stars) { if (stars >= 5) return 2; if (stars >= 4) return 1; return 0; }

function summonCardToSlot(slotIndex, rowType, isOccupied = false) {
    if (game.selectedHandIndex === null) return;
    const field = rowType === 'atk' ? game.playerAtkField : game.playerDefField;
    const card = game.playerHand[game.selectedHandIndex];
    if (card === undefined) return;
    const tribNeeded = getTributeCount(card.stars);

    if (tribNeeded === 0 && isOccupied) {
        showToast('Ô này đã có tướng!'); return;
    }

    if (tribNeeded > 0 && totalPlayerField() < tribNeeded) {
        showToast(`Cần ${tribNeeded} tướng trên sân để hiến tế!`); return;
    }

    pendingSummon = { slotIndex: slotIndex, handIndex: game.selectedHandIndex, card: card, row: rowType };

    if (tribNeeded > 0) {
        // Bắt đầu chọn hiến tế
        tributeState = {
            needed: tribNeeded, selected: [], callback: () => {
                // Xóa các quân hiến tế
                tributeState.selected.forEach(t => {
                    if (t.row === 'atk') game.playerAtkField[t.idx] = null;
                    else game.playerDefField[t.idx] = null;
                });
                doSummon(pendingSummon.row === 'atk' ? 'ATK' : 'DEF');
            }
        };

        if (isOccupied) {
            tributeState.selected.push({ idx: slotIndex, row: rowType });
        }

        if (tributeState.selected.length === tributeState.needed) {
            playSound(sfx.damage);
            showToast('💀 HIẾN TẾ!');
            setTimeout(() => { if (tributeState.callback) tributeState.callback(); tributeState = { needed: 0, selected: [], callback: null }; }, 600);
        } else {
            showToast(`⚠️ Chọn thêm ${tribNeeded - tributeState.selected.length} tướng để hiến tế!`);
            renderField();
        }
    } else {
        doSummon(rowType === 'atk' ? 'ATK' : 'DEF');
    }
}

function handleTributeClick(idx, row) {
    const key = row + '_' + idx;
    const existing = tributeState.selected.findIndex(t => t.row === row && t.idx === idx);
    if (existing !== -1) {
        tributeState.selected.splice(existing, 1);
    } else {
        if (tributeState.selected.length >= tributeState.needed) { showToast('Đã chọn đủ!'); return; }
        tributeState.selected.push({ idx: idx, row: row });
    }
    renderField();
    if (tributeState.selected.length === tributeState.needed) {
        // Đủ hiến tế - thực hiện
        playSound(sfx.damage);
        showToast('💀 HIẾN TẾ!');
        setTimeout(() => { if (tributeState.callback) tributeState.callback(); tributeState = { needed: 0, selected: [], callback: null }; }, 600);
    }
}

function doSummon(position) {
    if (!pendingSummon.card) return; playSound(sfx.summon);
    const card = game.playerHand.splice(pendingSummon.handIndex, 1)[0];
    card.faceDown = false; card.position = position; card.canAttack = true; card.hasChangedPosition = false;
    const field = pendingSummon.row === 'atk' ? game.playerAtkField : game.playerDefField;
    field[pendingSummon.slotIndex] = card;
    game.selectedHandIndex = null;
    showToast(position === 'ATK' ? '⚔️ HÀNG CÔNG' : '🛡️ HÀNG THỦ');
    pendingSummon = { slotIndex: null, handIndex: null, card: null, row: null };
    if (game.turn === 'player') addQuestProgress('summon_10', 1);
    renderHand(); renderField();
}
function confirmSummonDirect(position) { doSummon(position); }
function confirmSummon(position) { doSummon(position); }
function cancelSummon() { document.getElementById('summon-popup').style.display = 'none'; pendingSummon = { slotIndex: null, handIndex: null, card: null, row: null }; tributeState = { needed: 0, selected: [], callback: null }; renderField(); }

// === FIELD CLICK ===
function handleFieldClick(index, side, rowType) {
    if (isGameOver || game.isAnimating) return;
    // Battle phase: select attacker or target
    if (game.phase === 'your_battle' && game.turn === 'player') {
        if (side === 'player' && rowType === 'atk') {
            let card = game.playerAtkField[index]; if (!card) return;
            if (!card.canAttack) { showToast('Tướng này đã tấn công!'); return; }
            clearAttackHighlights();
            game.selectedCardIndex = index; game.selectedCardRow = 'atk';
            const dom = ui.pAtkZone[index].firstChild; if (dom) dom.style.boxShadow = '0 0 20px #e74c3c';
            // Direct attack if no enemy on field
            if (totalEnemyField() === 0) { executeBattle(index, 'atk', null, null); return; }
        } else if (side === 'enemy' && game.selectedCardIndex !== null) {
            // Must target DEF row first if it has cards
            if (rowType === 'atk' && rowCount(game.enemyDefField) > 0) { showToast('Phải phá hàng THỦ trước!'); return; }
            executeBattle(game.selectedCardIndex, game.selectedCardRow, index, rowType);
        } else if (side === 'player' && rowType === 'def') {
            showToast('Hàng THỦ không thể tấn công!');
        }
    }
}
function clearAttackHighlights() { for (let i = 0; i < 5; i++) { if (ui.pAtkZone[i].firstChild) ui.pAtkZone[i].firstChild.style.boxShadow = ''; } }

// === BATTLE ===
function executeBattle(atkIdx, atkRow, defIdx, defRow) {
    game.isAnimating = true; clearTurnTimer();
    let atkField = atkRow === 'atk' ? game.playerAtkField : game.playerDefField;
    let atkDOM = (atkRow === 'atk' ? ui.pAtkZone : ui.pDefZone)[atkIdx].firstChild;
    let attacker = atkField[atkIdx];
    if (!attacker) { game.isAnimating = false; startTurnTimer(); return; }
    attacker.canAttack = false; game.selectedCardIndex = null; game.selectedCardRow = null; clearAttackHighlights();
    if (defIdx === null) { takeDirectDamage('enemy', attacker.atk, atkDOM); return; }
    let dField = defRow === 'atk' ? game.enemyAtkField : game.enemyDefField;
    let dDOM = (defRow === 'atk' ? ui.eAtkZone : ui.eDefZone)[defIdx].firstChild;
    let defender = dField[defIdx];
    if (!defender) { game.isAnimating = false; startTurnTimer(); return; }
    if (defender.faceDown) { defender.faceDown = false; if (dDOM) { dDOM.style.backgroundImage = `url('${defender.img || CARD_BACK}')`; dDOM.classList.remove('face-down'); } playSound(sfx.phase); }
    startAttackAnim(atkDOM, dDOM, attacker, defender, true, atkField, atkIdx, dField, defIdx, defRow);
}

function startAttackAnim(atkDOM, defDOM, atkData, defData, isPlayerAtk, atkField, atkSlot, defField, defSlot, defRow) {
    if (atkDOM) { playSound(sfx.attack); atkDOM.classList.add('anim-attack'); }
    setTimeout(() => {
        if (atkDOM) atkDOM.classList.remove('anim-attack');
        const isDefRow = defRow === 'def';
        const defVal = isDefRow ? (defData.def || 0) : (defData.atk || 0);
        const atkVal = atkData.atk || 0;
        const dmg = Math.abs(atkVal - defVal);
        if (atkVal > defVal) {
            if (isDefRow) { destroyCard(defDOM, defField, defSlot); }
            else { takeDamage(isPlayerAtk ? 'enemy' : 'player', dmg); destroyCard(defDOM, defField, defSlot); }
        } else if (atkVal < defVal) {
            if (isDefRow) { takeDamage(isPlayerAtk ? 'player' : 'enemy', dmg); game.isAnimating = false; if (game.turn === 'player') startTurnTimer(); }
            else { takeDamage(isPlayerAtk ? 'player' : 'enemy', dmg); destroyCard(atkDOM, atkField, atkSlot); }
        } else {
            if (isDefRow) { showToast('Cân bằng!'); game.isAnimating = false; if (game.turn === 'player') startTurnTimer(); }
            else { let c = 0; const done = () => { c++; if (c >= 2) { game.isAnimating = false; if (game.turn === 'player') startTurnTimer(); } }; destroyCard(defDOM, defField, defSlot, done); destroyCard(atkDOM, atkField, atkSlot, done); return; }
        }
    }, 500);
}

function destroyCard(dom, field, slot, cb) {
    if (dom) dom.classList.add('anim-destroy'); playSound(sfx.summon);
    if (field === game.enemyAtkField || field === game.enemyDefField) {
        addQuestProgress('destroy_5', 1);
    }
    setTimeout(() => { if (slot !== undefined && slot !== null) field[slot] = null; renderField(); if (cb) cb(); else { game.isAnimating = false; if (game.turn === 'player') startTurnTimer(); } }, 600);
}
function takeDamage(target, amount) {
    playSound(sfx.damage);
    let scr = document.getElementById('main-area'); if (scr) { scr.classList.add('shake-screen'); setTimeout(() => scr.classList.remove('shake-screen'), 500); }
    showDamageNumber(target, amount);
    if (target === 'enemy') {
        game.enemyLp -= amount;
        addQuestProgress('damage_5000', amount);
    } else {
        game.playerLp -= amount;
    }
    updateLP(); checkWin();
}
function takeDirectDamage(target, amount, atkDOM) {
    playSound(sfx.attack); if (atkDOM) atkDOM.classList.add('anim-attack');
    setTimeout(() => { if (atkDOM) atkDOM.classList.remove('anim-attack'); takeDamage(target, amount); game.isAnimating = false; if (game.turn === 'player') startTurnTimer(); }, 500);
}

// === WIN/LOSE ===
function checkWin() {
    if (isGameOver) return;
    if (game.enemyLp <= 0 || game.enemyDeck.length === 0) { isGameOver = true; clearTurnTimer(); playSound(sfx.victory); showGameOver(true); }
    else if (game.playerLp <= 0 || game.playerDeck.length === 0) { isGameOver = true; clearTurnTimer(); playSound(sfx.defeat); showGameOver(false); }
}
function showGameOver(win) {
    const m = document.getElementById('game-over-modal'), ic = document.getElementById('game-over-icon'), ti = document.getElementById('game-over-title'), ms = document.getElementById('game-over-message');
    if (win) {
        ic.innerText = '🎉'; ti.innerText = 'CHIẾN THẮNG!'; ti.className = 'game-over-title victory';
        ms.innerText = 'Bạn là bá chủ thiên hạ!';

        if (gameMode === 'story') {
            const story = storyData.find(s => s.id === activeStoryId);
            if (story) {
                if (!completedStories.includes(story.id)) {
                    completedStories.push(story.id);
                }
                let rewardGrid = document.getElementById('reward-cards-grid');
                rewardGrid.innerHTML = '';
                story.rewardCards.forEach(cid => {
                    if (!unlockedCards.includes(cid)) {
                        unlockedCards.push(cid);
                    }
                    const cw = document.createElement('div');
                    cw.className = 'gacha-result-card';
                    const c = cardDb.find(x => x.id === cid);
                    if (c) cw.style.backgroundImage = `url('${c.img}')`;
                    rewardGrid.appendChild(cw);
                });
                playerGold += story.goldReward;
                document.getElementById('reward-gold-amount').innerText = story.goldReward;
                document.getElementById('reward-story-name').innerText = story.title;
                saveProgress();
                setTimeout(() => {
                    document.getElementById('game-over-modal').style.display = 'none';
                    document.getElementById('story-reward-modal').style.display = 'flex';
                }, 1500);
            }
        } else if (gameMode === 'ai') {
            addQuestProgress('win_ai', 1);
            let goldReward = 0;
            if (aiDifficulty === 'easy') goldReward = 20;
            else if (aiDifficulty === 'medium') goldReward = 50;
            else if (aiDifficulty === 'hard') goldReward = 100;

            playerGold += goldReward;
            saveProgress();
            ms.innerHTML = `Bạn là bá chủ thiên hạ!<br><br><span style="color:#f1c40f; font-size: 26px; font-weight: bold; text-shadow:0 2px 5px #000;">+${goldReward} Vàng 🪙</span>`;
        }
    }
    else { ic.innerText = '💀'; ti.innerText = 'THẤT BẠI!'; ti.className = 'game-over-title defeat'; ms.innerText = 'Hãy tập hợp lại binh mã!'; }
    m.style.display = 'flex';
}
function replayGame() { document.getElementById('game-over-modal').style.display = 'none'; Object.values(sfx).forEach(s => { s.pause(); s.currentTime = 0; }); initGame(); }
function goToDeckBuilder() { document.getElementById('game-over-modal').style.display = 'none'; Object.values(sfx).forEach(s => { s.pause(); s.currentTime = 0; }); showDeckBuilder(); }

// === AI TURN ===
function endTurn() {
    clearTurnTimer();
    if (gameMode === 'pvp') { showPvPHandoff(); return; }
    game.turn = 'enemy';
    setPhase('enemy_main', 'ĐỊCH HÀNH ĐỘNG');
    if (game.enemyDeck.length > 0) {
        const c = game.enemyDeck.pop();
        c.faceDown = true;
        animateCardDraw(false, c, () => {
            setTimeout(enemyMainPhase, 800);
        });
    } else {
        setTimeout(enemyMainPhase, 800);
    }
}

function enemyMainPhase() {
    if (isGameOver) return;
    const enemyTotal = rowCount(game.enemyAtkField) + rowCount(game.enemyDefField);
    if (game.enemyHand.length > 0 && enemyTotal < 10) {
        // AI chọn bài để triệu hồi
        let cardIdx = -1;
        if (aiDifficulty === 'easy') { cardIdx = Math.floor(Math.random() * game.enemyHand.length); }
        else { let best = -1; game.enemyHand.forEach((c, i) => { if (c.atk > best) { best = c.atk; cardIdx = i; } }); if (cardIdx === -1) cardIdx = 0; }
        let card = game.enemyHand[cardIdx];
        let tribNeeded = getTributeCount(card.stars);
        // Kiểm tra đủ quân hiến tế không
        if (tribNeeded > enemyTotal) {
            // Thử tìm bài không cần hiến tế
            let fallback = game.enemyHand.findIndex(c => getTributeCount(c.stars) <= enemyTotal);
            if (fallback !== -1) { cardIdx = fallback; card = game.enemyHand[cardIdx]; tribNeeded = getTributeCount(card.stars); }
            else { setTimeout(enemyBattlePhase, 1500); return; } // Không đủ, bỏ qua
        }
        // Hiến tế AI: xóa quân yếu nhất
        if (tribNeeded > 0) {
            let allCards = [];
            for (let i = 0; i < 5; i++) { if (game.enemyAtkField[i]) allCards.push({ idx: i, row: 'atk', power: game.enemyAtkField[i].atk }); if (game.enemyDefField[i]) allCards.push({ idx: i, row: 'def', power: game.enemyDefField[i].def }); }
            allCards.sort((a, b) => a.power - b.power); // yếu nhất trước
            for (let t = 0; t < tribNeeded && t < allCards.length; t++) {
                let tr = allCards[t];
                if (tr.row === 'atk') game.enemyAtkField[tr.idx] = null; else game.enemyDefField[tr.idx] = null;
            }
        }
        game.enemyHand.splice(cardIdx, 1);
        card.faceDown = true; card.canAttack = true; card.hasChangedPosition = false;
        let targetRow, targetField;
        if (aiDifficulty === 'easy') { targetRow = Math.random() < 0.5 ? 'atk' : 'def'; }
        else if (aiDifficulty === 'hard') { targetRow = card.type === 'vo' ? 'atk' : 'def'; }
        else { targetRow = Math.random() < 0.7 ? 'atk' : 'def'; }
        targetField = targetRow === 'atk' ? game.enemyAtkField : game.enemyDefField;
        card.position = targetRow === 'atk' ? 'ATK' : 'DEF';
        let empty = []; for (let i = 0; i < 5; i++)if (targetField[i] === null) empty.push(i);
        if (empty.length === 0) { targetField = targetRow === 'atk' ? game.enemyDefField : game.enemyAtkField; empty = []; for (let i = 0; i < 5; i++)if (targetField[i] === null) empty.push(i); }
        if (empty.length > 0) targetField[empty[Math.floor(Math.random() * empty.length)]] = card;
        playSound(sfx.summon); renderEnemyHand(); renderField();
    }
    setTimeout(enemyBattlePhase, 1500);
}

function enemyBattlePhase() {
    if (isGameOver) return; setPhase('enemy_battle', 'ĐỊCH TẤN CÔNG');
    let attackers = [];
    for (let i = 0; i < 5; i++)if (game.enemyAtkField[i] && game.enemyAtkField[i].canAttack) attackers.push(i);
    if (attackers.length === 0) { setTimeout(finishEnemyTurn, 1000); return; }
    executeEnemyAttacks(attackers, 0);
}

function executeEnemyAttacks(attackers, idx) {
    if (idx >= attackers.length || isGameOver) { setTimeout(finishEnemyTurn, 1000); return; }
    let slot = attackers[idx];
    let aiCard = game.enemyAtkField[slot];
    if (!aiCard) { executeEnemyAttacks(attackers, idx + 1); return; }
    let atkDOM = ui.eAtkZone[slot].firstChild;
    if (aiCard.faceDown) { aiCard.faceDown = false; if (atkDOM) { atkDOM.style.backgroundImage = `url('${aiCard.img || CARD_BACK}')`; atkDOM.classList.remove('face-down'); } }
    // Find target
    if (rowCount(game.playerDefField) > 0) {
        let ti = pickAITarget(game.playerDefField, 'def');
        if (ti !== null) {
            let tDOM = ui.pDefZone[ti].firstChild;
            doEnemyAttack(atkDOM, tDOM, aiCard, game.playerDefField[ti], game.enemyAtkField, slot, game.playerDefField, ti, 'def', () => executeEnemyAttacks(attackers, idx + 1));
            return;
        }
    }
    if (rowCount(game.playerAtkField) > 0) {
        let ti = pickAITarget(game.playerAtkField, 'atk');
        if (ti !== null) {
            let tDOM = ui.pAtkZone[ti].firstChild;
            doEnemyAttack(atkDOM, tDOM, aiCard, game.playerAtkField[ti], game.enemyAtkField, slot, game.playerAtkField, ti, 'atk', () => executeEnemyAttacks(attackers, idx + 1));
            return;
        }
    }
    // Direct attack
    playSound(sfx.attack); if (atkDOM) atkDOM.classList.add('anim-attack');
    setTimeout(() => { if (atkDOM) atkDOM.classList.remove('anim-attack'); takeDamage('player', aiCard.atk); setTimeout(() => executeEnemyAttacks(attackers, idx + 1), 500); }, 500);
}

function pickAITarget(field, rowType) {
    let candidates = []; for (let i = 0; i < 5; i++)if (field[i]) candidates.push(i);
    if (candidates.length === 0) return null;
    if (aiDifficulty === 'easy') return candidates[Math.floor(Math.random() * candidates.length)];
    // Medium/Hard: target weakest
    let best = null, lowest = Infinity;
    candidates.forEach(i => { let val = rowType === 'def' ? field[i].def : field[i].atk; if (val < lowest) { lowest = val; best = i; } });
    return best;
}

function doEnemyAttack(atkDOM, defDOM, atkData, defData, atkField, atkSlot, defField, defSlot, defRow, cb) {
    game.isAnimating = true;
    if (atkDOM) { playSound(sfx.attack); atkDOM.classList.add('anim-attack'); }
    setTimeout(() => {
        if (atkDOM) atkDOM.classList.remove('anim-attack');
        if (defData.faceDown) { defData.faceDown = false; if (defDOM) { defDOM.style.backgroundImage = `url('${defData.img || CARD_BACK}')`; defDOM.classList.remove('face-down'); } }
        const isDefRow = defRow === 'def';
        const defVal = isDefRow ? (defData.def || 0) : (defData.atk || 0);
        const atkVal = atkData.atk || 0; const dmg = Math.abs(atkVal - defVal);
        if (atkVal > defVal) {
            if (!isDefRow) takeDamage('player', dmg);
            if (defDOM) defDOM.classList.add('anim-destroy'); playSound(sfx.summon);
            setTimeout(() => { defField[defSlot] = null; renderField(); game.isAnimating = false; setTimeout(cb, 400); }, 600);
        } else if (atkVal < defVal) {
            if (isDefRow) takeDamage('enemy', dmg); else { takeDamage('enemy', dmg); if (atkDOM) atkDOM.classList.add('anim-destroy'); setTimeout(() => { atkField[atkSlot] = null; renderField(); }, 600); }
            game.isAnimating = false; setTimeout(cb, 800);
        } else {
            if (!isDefRow) { if (defDOM) defDOM.classList.add('anim-destroy'); if (atkDOM) atkDOM.classList.add('anim-destroy'); setTimeout(() => { defField[defSlot] = null; atkField[atkSlot] = null; renderField(); game.isAnimating = false; setTimeout(cb, 400); }, 600); }
            else { showToast('Cân bằng!'); game.isAnimating = false; setTimeout(cb, 400); }
        }
    }, 500);
}

function finishEnemyTurn() {
    if (isGameOver) return; game.isAnimating = false;
    setPhase('enemy_turn_over', 'HẾT LƯỢT ĐỊCH'); setTimeout(() => nextPhase(), 1000);
}

// === PVP ===
function showPvPHandoff() {
    const m = document.getElementById('pvp-handoff-modal');
    const nextP = currentPlayer === 1 ? 2 : 1;
    document.getElementById('handoff-title').innerText = 'CHUYỂN LƯỢT';
    document.getElementById('handoff-message').innerText = `Hãy chuyển máy cho Người Chơi ${nextP}`;
    m.style.display = 'flex';
}
function confirmHandoff() {
    document.getElementById('pvp-handoff-modal').style.display = 'none';
    // Swap all data
    [game.playerHand, game.enemyHand] = [game.enemyHand, game.playerHand];
    [game.playerAtkField, game.enemyAtkField] = [game.enemyAtkField, game.playerAtkField];
    [game.playerDefField, game.enemyDefField] = [game.enemyDefField, game.playerDefField];
    [game.playerDeck, game.enemyDeck] = [game.enemyDeck, game.playerDeck];
    [game.playerLp, game.enemyLp] = [game.enemyLp, game.playerLp];
    currentPlayer = currentPlayer === 1 ? 2 : 1;
    game.turn = 'player'; game.selectedHandIndex = null; game.selectedCardIndex = null; game.isAnimating = false;
    document.getElementById('ui-player-label').innerText = 'P' + currentPlayer;
    document.getElementById('ui-enemy-label').innerText = 'P' + (currentPlayer === 1 ? 2 : 1);
    setPhase('pvp_ready', 'BẤM TIẾP THEO ĐỂ BẮT ĐẦU LƯỢT!');
    game.playerAtkField.forEach(c => { if (c) { c.canAttack = true; c.hasChangedPosition = false; } });
    game.playerDefField.forEach(c => { if (c) { c.canAttack = true; c.hasChangedPosition = false; } });
    updateLP(); updateDeckCounts(); renderHand(); renderEnemyHand(); renderField();
    ui.btnNextPhase.disabled = false;
}

// === PHASE ===
function nextPhase() {
    if (isGameOver) return; if (game.isAnimating && game.phase !== 'enemy_turn_over') return;
    playSound(sfx.phase); game.selectedHandIndex = null;
    switch (game.phase) {
        case 'standby': setPhase('dealing', 'CHIA BÀI ĐẦU TRẬN'); drawInitialHands(); break;
        case 'dealing': break;
        case 'pvp_ready': setPhase('your_main1', 'GIAI ĐOẠN CHÍNH 1'); break;
        case 'your_main1': setPhase('your_battle', 'CHIẾN ĐẤU'); break;
        case 'your_battle': setPhase('your_main2', 'GIAI ĐOẠN CHÍNH 2'); break;
        case 'your_main2': setPhase('end_turn', 'KẾT THÚC LƯỢT'); endTurn(); break;
        case 'enemy_turn_over':
            game.turn = 'player'; game.isAnimating = false;
            game.selectedCardIndex = null; game.selectedHandIndex = null;
            setPhase('your_main1', 'GIAI ĐOẠN CHÍNH 1');
            game.playerAtkField.forEach(c => { if (c) { c.canAttack = true; } });
            game.playerDefField.forEach(c => { if (c) { c.canAttack = true; } });
            ui.btnNextPhase.disabled = false; break;
    }
}
function setPhase(pid, text) {
    game.phase = pid; ui.phaseText.innerText = text;
    ui.btnNextPhase.innerText = (pid === 'your_main2') ? 'Hết Lượt' : (pid === 'standby' || pid === 'pvp_ready') ? 'Bắt Đầu Trận' : 'Tiếp Theo';
    ui.btnNextPhase.disabled = ['dealing', 'enemy_main', 'enemy_battle', 'enemy_turn_over'].includes(pid);
    if (['your_main1', 'your_battle', 'your_main2'].includes(pid)) startTurnTimer(); else clearTurnTimer();
    renderField(); renderHand();
}
// === DECK BUILDER ===
function showDeckBuilder() { document.getElementById('deck-builder-screen').style.display = 'flex'; renderDeckBuilder(); }
let currentKingdomFilter = 'all';
function setKingdomFilter(k) {
    currentKingdomFilter = k;
    document.querySelectorAll('.db-tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.db-tab').forEach(t => {
        if (t.dataset.kingdom === k) t.classList.add('active');
    });
    renderDeckBuilder();
}

function renderDeckBuilder() {
    const grid = document.getElementById('card-collection-grid'); grid.innerHTML = '';

    const searchVal = document.getElementById('db-search').value.toLowerCase();
    const starVal = document.getElementById('db-filter-star').value;
    const typeVal = document.getElementById('db-filter-type').value;
    const atkVal = document.getElementById('db-filter-atk') ? document.getElementById('db-filter-atk').value : 'all';
    const sortVal = document.getElementById('db-sort').value;

    let filtered = cardDb.filter(c => {
        if (currentKingdomFilter !== 'all' && c.kingdom !== currentKingdomFilter) return false;
        if (starVal !== 'all' && c.stars.toString() !== starVal) return false;
        if (typeVal !== 'all' && c.type !== typeVal) return false;
        if (atkVal !== 'all') {
            if (atkVal === 'high' && c.atk < 2500) return false;
            if (atkVal === 'medium' && (c.atk < 1500 || c.atk >= 2500)) return false;
            if (atkVal === 'low' && c.atk >= 1500) return false;
        }
        if (searchVal) {
            const normalize = (s) => s.normalize("NFD").replace(/[\u0300-\u036f]/g, "").replace(/đ/g, "d").replace(/Đ/g, "D").toLowerCase();
            if (!normalize(c.name).includes(normalize(searchVal))) return false;
        }
        return true;
    });

    filtered.sort((a, b) => {
        if (sortVal === 'star') return (b.stars || 0) - (a.stars || 0) || b.atk - a.atk;
        if (sortVal === 'atk') return b.atk - a.atk || (b.stars || 0) - (a.stars || 0);
        if (sortVal === 'def') return b.def - a.def || (b.stars || 0) - (a.stars || 0);
        if (sortVal === 'name') return a.name.localeCompare(b.name);
        return 0;
    });

    filtered.forEach(card => {
        const isUnlocked = gameMode === 'ai' ? true : unlockedCards.includes(card.id);
        const wrapper = document.createElement('div');
        wrapper.className = 'db-card-wrapper' + (!isUnlocked ? ' locked' : '');
        wrapper.onclick = () => {
            if (!isUnlocked) {
                showToast('🔒 Thẻ này chưa mở khóa! Hãy vào Cửa Hàng để rút thẻ!');
                return;
            }
            toggleCardInDeck(card.id);
        };

        const el = document.createElement('div'); el.className = 'db-card ' + getStarColorClass(card.stars);
        if (!isUnlocked) el.classList.add('card-locked-style');
        if (selectedPlayerDeck.includes(card.id)) el.classList.add('selected');
        el.style.backgroundImage = `url('${card.img}')`;

        const info = document.createElement('div'); info.className = 'db-card-info';
        info.innerHTML = `<div class="db-card-type type-${card.type}">${getTypeLabel(card.type)}</div><div class="db-card-stars-row">${getStarsDisplay(card.stars)}</div><div class="db-card-stats"><span class="db-atk">⚔${card.atk}</span>/<span class="db-def">🛡${card.def}</span></div>`;
        el.appendChild(info);

        const nameLabel = document.createElement('div');
        nameLabel.className = 'db-card-name-below';
        nameLabel.innerText = card.name;

        wrapper.appendChild(el);
        wrapper.appendChild(nameLabel);

        wrapper.onmouseenter = () => { const bar = document.getElementById('db-info-bar'); bar.style.color = '#f1c40f'; bar.innerText = `${getStarsDisplay(card.stars)} ${card.name} [${getTypeLabel(card.type)}] — ATK:${card.atk} / DEF:${card.def} — ${card.desc}`; };
        wrapper.onmouseleave = () => { const bar = document.getElementById('db-info-bar'); bar.style.color = '#7f8c8d'; bar.innerText = 'Di chuột vào thẻ bài để xem chi tiết tướng'; };

        grid.appendChild(wrapper);
    });
    updateDeckBuilderCount();
}
function toggleCardInDeck(id) { const i = selectedPlayerDeck.indexOf(id); if (i !== -1) selectedPlayerDeck.splice(i, 1); else { if (selectedPlayerDeck.length >= 30) { showToast('Đã đủ 30 lá!'); return; } selectedPlayerDeck.push(id); } renderDeckBuilder(); }
function updateDeckBuilderCount() { const c = document.getElementById('deck-selected-count'), b = document.getElementById('btn-start-game'); if (c) c.innerText = selectedPlayerDeck.length; if (b) b.disabled = selectedPlayerDeck.length < 20; }
function randomSelectDeck() {
    selectedPlayerDeck = [];
    const pool = gameMode === 'ai' ? cardDb : cardDb.filter(c => unlockedCards.includes(c.id));
    const s = shuffleArray([...pool]);
    for (let i = 0; i < 30 && i < s.length; i++)selectedPlayerDeck.push(s[i].id);
    renderDeckBuilder(); showToast('🎲 Ngẫu nhiên 30 lá!');
}
function clearDeckSelection() { selectedPlayerDeck = []; renderDeckBuilder(); showToast('❌ Đã xóa hết!'); }

function startGameWithDeck() {
    if (selectedPlayerDeck.length < 20) { showToast('Cần ít nhất 20 lá!'); return; }
    if (gameMode === 'pvp' && pvpBuildPhase === 1) {
        p1Deck = [...selectedPlayerDeck]; selectedPlayerDeck = []; pvpBuildPhase = 2;
        document.getElementById('deck-builder-title').innerText = '⚔️ NGƯỜI CHƠI 2 - CHỌN BÀI ⚔️';
        renderDeckBuilder(); showToast('Người chơi 2, hãy chọn bài!'); return;
    }
    if (gameMode === 'pvp' && pvpBuildPhase === 2) { p2Deck = [...selectedPlayerDeck]; }
    document.getElementById('deck-builder-screen').style.display = 'none';
    initGame();
}

// === INIT ===
function createDeckFromIds(ids) { return shuffleArray(ids.map(id => { const t = cardDb.find(c => c.id === id); return { ...t }; })); }
function createEnemyDeck() { return shuffleArray(cardDb.map(c => ({ ...c }))).slice(0, DECK_SIZE); }

function createStoryEnemyDeck() {
    const s = storyData.find(x => x.id === activeStoryId);
    if(!s) return createEnemyDeck();
    let arr = [];
    s.enemyCards.forEach(id => { let card = cardDb.find(c => c.id === id); if(card) arr.push({...card}); });
    let idx = 0;
    while(arr.length < DECK_SIZE && s.enemyCards.length > 0) {
        let card = cardDb.find(c => c.id === s.enemyCards[idx % s.enemyCards.length]);
        if(card) arr.push({...card});
        idx++;
    }
    return shuffleArray(arr);
}

function createStoryPlayerDeck() {
    const s = storyData.find(x => x.id === activeStoryId);
    if(!s || !s.playerCards) return createDeckFromIds(unlockedCards.slice(0, DECK_SIZE));
    let arr = [];
    s.playerCards.forEach(id => { let card = cardDb.find(c => c.id === id); if(card) arr.push({...card}); });
    let idx = 0;
    while(arr.length < DECK_SIZE && s.playerCards.length > 0) {
        let card = cardDb.find(c => c.id === s.playerCards[idx % s.playerCards.length]);
        if(card) arr.push({...card});
        idx++;
    }
    return shuffleArray(arr);
}
function shuffleArray(arr) { for (let i = arr.length - 1; i > 0; i--) { const j = Math.floor(Math.random() * (i + 1));[arr[i], arr[j]] = [arr[j], arr[i]]; } return arr; }

function initGame() {
    game.playerLp = MAX_LP; game.enemyLp = MAX_LP; game.turn = 'player'; game.phase = 'standby';
    game.playerHand = []; game.enemyHand = [];
    game.playerAtkField = [null, null, null, null, null]; game.playerDefField = [null, null, null, null, null];
    game.enemyAtkField = [null, null, null, null, null]; game.enemyDefField = [null, null, null, null, null];
    game.selectedHandIndex = null; game.selectedCardIndex = null; game.selectedCardRow = null;
    game.isAnimating = false; isGameOver = false; currentPlayer = 1;
    pendingSummon = { slotIndex: null, handIndex: null, card: null, row: null };
    if (gameMode === 'pvp') {
        game.playerDeck = createDeckFromIds(p1Deck); game.enemyDeck = createDeckFromIds(p2Deck);
        document.getElementById('ui-player-label').innerText = 'P1';
        document.getElementById('ui-enemy-label').innerText = 'P2';
    } else if (gameMode === 'story') {
        const s = storyData.find(x => x.id === activeStoryId);
        game.playerDeck = createStoryPlayerDeck(); game.enemyDeck = createStoryEnemyDeck();
        aiDifficulty = s.difficulty;
        document.getElementById('ui-player-label').innerText = 'BẠN';
        document.getElementById('ui-enemy-label').innerText = s.enemyName.toUpperCase();
    } else {
        game.playerDeck = createDeckFromIds(selectedPlayerDeck); game.enemyDeck = createEnemyDeck();
        document.getElementById('ui-player-label').innerText = 'BẠN';
        document.getElementById('ui-enemy-label').innerText = 'ĐỊCH';
    }
    ui.phaseText.innerText = 'CHUẨN BỊ CHIẾN ĐẤU!'; ui.btnNextPhase.innerText = 'Bắt Đầu Trận'; ui.btnNextPhase.disabled = false;
    updateLP(); updateHPBars(); updateDeckCounts(); renderHand(); renderEnemyHand(); renderField();
}

// === START ===
document.addEventListener('DOMContentLoaded', () => { initProgression(); initUI(); });