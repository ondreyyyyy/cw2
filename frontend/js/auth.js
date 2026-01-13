// Authentication state management

// Update UI based on auth state
function updateAuthUI() {
    const loginBtn = document.getElementById('loginBtn');
    const userSection = document.getElementById('userSection');
    const usernameSpan = document.getElementById('username');
    
    if (!loginBtn || !userSection) return;
    
    const user = getCurrentUser();
    
    if (user) {
        loginBtn.style.display = 'none';
        userSection.style.display = 'flex';
        
        // Добавляем ссылки для пользователя
        let usernameHTML = `<a href="/pages/change-password.html" style="color: white; text-decoration: none;">${user.login}</a>`;
        
        // Добавляем ссылку на админ-панель для администратора
        if (user.isAdmin) {
            usernameHTML = `<a href="/pages/admin.html" style="color: #008000; text-decoration: none; margin-right: 1rem;">Админ-панель</a>` + usernameHTML;
        }
        
        usernameSpan.innerHTML = usernameHTML;
    } else {
        loginBtn.style.display = 'block';
        userSection.style.display = 'none';
    }
}

// Handle logout
function handleLogout() {
    removeAuthToken();
    removeCurrentUser();
    updateAuthUI();
    window.location.href = '/index.html';
}

// Setup logout button
function setupLogoutButton() {
    const logoutBtn = document.getElementById('logoutBtn');
    if (logoutBtn) {
        logoutBtn.addEventListener('click', handleLogout);
    }
}

// Check authentication and redirect if needed
function requireAuth() {
    if (!isAuthenticated()) {
        window.location.href = '/pages/login.html';
        return false;
    }
    return true;
}

// Initialize auth on page load
document.addEventListener('DOMContentLoaded', () => {
    updateAuthUI();
    setupLogoutButton();
});
