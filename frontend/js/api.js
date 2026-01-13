// API configuration
const API_BASE_URL = '/api';

// Helper function to get auth token
function getAuthToken() {
    return localStorage.getItem('authToken');
}

// Helper function to set auth token
function setAuthToken(token) {
    localStorage.setItem('authToken', token);
}

// Helper function to remove auth token
function removeAuthToken() {
    localStorage.removeItem('authToken');
}

// Helper function to get current user
function getCurrentUser() {
    const userStr = localStorage.getItem('currentUser');
    return userStr ? JSON.parse(userStr) : null;
}

// Helper function to set current user
function setCurrentUser(user) {
    localStorage.setItem('currentUser', JSON.stringify(user));
}

// Helper function to remove current user
function removeCurrentUser() {
    localStorage.removeItem('currentUser');
}

// Check if user is authenticated
function isAuthenticated() {
    return !!getAuthToken();
}

// API request helper
async function apiRequest(endpoint, options = {}) {
    const token = getAuthToken();
    
    const defaultHeaders = {
        'Content-Type': 'application/json'
    };
    
    if (token) {
        defaultHeaders['Authorization'] = `Bearer ${token}`;
    }
    
    const config = {
        ...options,
        headers: {
            ...defaultHeaders,
            ...options.headers
        }
    };
    
    try {
        const response = await fetch(`${API_BASE_URL}${endpoint}`, config);
        const data = await response.json();
        
        // Handle unauthorized
        if (response.status === 401) {
            removeAuthToken();
            removeCurrentUser();
            window.location.href = '/pages/login.html';
            return null;
        }
        
        return data;
    } catch (error) {
        console.error('API request failed:', error);
        throw error;
    }
}

// Auth API
const authAPI = {
    async login(login, password) {
        return await apiRequest('/login', {
            method: 'POST',
            body: JSON.stringify({ login, password })
        });
    },
    
    async register(login, email, fullName, password, verificationCode) {
        return await apiRequest('/register', {
            method: 'POST',
            body: JSON.stringify({ login, email, fullName, password, verificationCode })
        });
    },
    
    async sendVerificationCode(email) {
        return await apiRequest('/send-verification-code', {
            method: 'POST',
            body: JSON.stringify({ email })
        });
    },
    
    async verifyCode(email, code) {
        return await apiRequest('/verify-code', {
            method: 'POST',
            body: JSON.stringify({ email, code })
        });
    },
    
    async verifyUserExists(login, email) {
        return await apiRequest('/verify-user-exists', {
            method: 'POST',
            body: JSON.stringify({ login, email })
        });
    },
    
    async resetPassword(login, email, newPassword) {
        return await apiRequest('/reset-password', {
            method: 'POST',
            body: JSON.stringify({ login, email, newPassword })
        });
    },
    
    async recoverPassword(login, email) {
        return await apiRequest('/recover-password', {
            method: 'POST',
            body: JSON.stringify({ login, email })
        });
    },
    
    async changePassword(oldPassword, newPassword) {
        return await apiRequest('/change-password', {
            method: 'POST',
            body: JSON.stringify({ oldPassword, newPassword })
        });
    },
    
    async getMe() {
        return await apiRequest('/me');
    }
};

// Events API
const eventsAPI = {
    async getAll(filters = {}) {
        const params = new URLSearchParams();
        Object.entries(filters).forEach(([key, value]) => {
            if (value) params.append(key, value);
        });
        const queryString = params.toString();
        return await apiRequest(`/events${queryString ? '?' + queryString : ''}`);
    },
    
    async getById(id) {
        return await apiRequest(`/event?id=${id}`);
    },
    
    async getCategories(eventId) {
        return await apiRequest(`/categories?eventId=${eventId}`);
    },
    
    async getAvailableTickets(eventId, categoryId = null) {
        let url = `/tickets/available?eventId=${eventId}`;
        if (categoryId) url += `&categoryId=${categoryId}`;
        return await apiRequest(url);
    },
    
    async getGenres() {
        return await apiRequest('/genres');
    },
    
    async getCities() {
        return await apiRequest('/cities');
    },
    
    async getVenues() {
        return await apiRequest('/venues');
    }
};

// Bookings API
const bookingsAPI = {
    async book(ticketId, eventId) {
        return await apiRequest('/book', {
            method: 'POST',
            body: JSON.stringify({ ticketId, eventId })
        });
    },
    
    async cancel(bookingId) {
        return await apiRequest(`/book?bookingId=${bookingId}`, {
            method: 'DELETE'
        });
    },
    
    async getAll(filters = {}) {
        const params = new URLSearchParams();
        Object.entries(filters).forEach(([key, value]) => {
            if (value) params.append(key, value);
        });
        const queryString = params.toString();
        return await apiRequest(`/bookings${queryString ? '?' + queryString : ''}`);
    },
    
    async getById(id) {
        return await apiRequest(`/booking?id=${id}`);
    }
};

// Admin API
const adminAPI = {
    async getVenuesByCity(city) {
        return await apiRequest(`/admin/venues?city=${encodeURIComponent(city)}`);
    },
    
    async getVenueCategories(venueId) {
        return await apiRequest(`/admin/venue-categories?venueId=${venueId}`);
    },
    
    async createEvent(eventData) {
        return await apiRequest('/admin/event', {
            method: 'POST',
            body: JSON.stringify(eventData)
        });
    },
    
    async getEvent(eventId) {
        return await apiRequest(`/admin/event?id=${eventId}`);
    },
    
    async updateEvent(eventData) {
        return await apiRequest('/admin/event', {
            method: 'PUT',
            body: JSON.stringify(eventData)
        });
    },
    
    async deleteEvent(eventId) {
        return await apiRequest(`/admin/event?id=${eventId}`, {
            method: 'DELETE'
        });
    }
};
