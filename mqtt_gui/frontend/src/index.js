import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';  // Import routing components
import App from './App';
import Config from './Config';
import AutoConfig from './AutoConfig';


const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <React.StrictMode>
    <Router>
      <Routes>
        {/* Define routes for App and Config components */}
        <Route path="/" element={<App />} />
        <Route path="/config" element={<Config />} />
        <Route path="/autoconfig" element={<AutoConfig />} />
      </Routes>
    </Router>
  </React.StrictMode>
);
