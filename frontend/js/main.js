// Main page functionality

document.addEventListener('DOMContentLoaded', () => {
    // Handle "Start booking" button
    const startBtn = document.getElementById('startBtn');
    if (startBtn) {
        startBtn.addEventListener('click', () => {
            if (isAuthenticated()) {
                window.location.href = '/pages/events.html';
            } else {
                window.location.href = '/pages/login.html';
            }
        });
    }
});
