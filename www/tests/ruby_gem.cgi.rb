#!/usr/bin/env ruby

# Ruby CGI script
require 'cgi'
require 'json'

cgi = CGI.new

puts cgi.header('text/html')

puts <<HTML
<!DOCTYPE html>
<html>
<head>
    <title>Ruby CGI Demo</title>
    <style>
        body { 
            font-family: 'Segoe UI', sans-serif; 
            margin: 20px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .container { max-width: 700px; margin: 0 auto; }
        .ruby-gem { 
            background: rgba(255,255,255,0.1); 
            padding: 25px; 
            border-radius: 15px; 
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
        }
        .gem-section { 
            background: rgba(255,255,255,0.05); 
            padding: 15px; 
            margin: 15px 0; 
            border-radius: 8px; 
            border-left: 4px solid #ff6b6b;
        }
        .code-block { 
            background: #2d3748; 
            color: #e2e8f0; 
            padding: 15px; 
            border-radius: 8px; 
            font-family: 'Courier New', monospace;
            overflow-x: auto;
        }
        .highlight { color: #ffd700; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <div class="ruby-gem">
            <h1>💎 Ruby CGI Script</h1>
            <p><span class="highlight">Language:</span> Ruby #{RUBY_VERSION}</p>
            <p><span class="highlight">Platform:</span> #{RUBY_PLATFORM}</p>
            
            <div class="gem-section">
                <h3>🌐 HTTP Request Details</h3>
                <p><span class="highlight">Method:</span> #{ENV['REQUEST_METHOD'] || 'Unknown'}</p>
                <p><span class="highlight">Query:</span> #{ENV['QUERY_STRING'] || 'None'}</p>
                <p><span class="highlight">Content Type:</span> #{ENV['CONTENT_TYPE'] || 'Not specified'}</p>
                <p><span class="highlight">User Agent:</span> #{ENV['HTTP_USER_AGENT']&.slice(0, 60) || 'Unknown'}...</p>
                <p><span class="highlight">Remote Address:</span> #{ENV['REMOTE_ADDR'] || 'Unknown'}</p>
            </div>
            
            <div class="gem-section">
                <h3>⚙️ Server Environment</h3>
                <p><span class="highlight">Server:</span> #{ENV['SERVER_NAME']}:#{ENV['SERVER_PORT']}</p>
                <p><span class="highlight">Script Name:</span> #{ENV['SCRIPT_NAME']}</p>
                <p><span class="highlight">Document Root:</span> #{ENV['DOCUMENT_ROOT']}</p>
                <p><span class="highlight">Gateway Interface:</span> #{ENV['GATEWAY_INTERFACE']}</p>
            </div>
            
            <div class="gem-section">
                <h3>🕰️ Runtime Information</h3>
                <p><span class="highlight">Current Time:</span> #{Time.now.strftime('%Y-%m-%d %H:%M:%S %Z')}</p>
                <p><span class="highlight">Process ID:</span> #{Process.pid}</p>
                <p><span class="highlight">Ruby Executable:</span> #{RbConfig.ruby}</p>
                <p><span class="highlight">Load Path Count:</span> #{$LOAD_PATH.length} directories</p>
            </div>
            
            <div class="gem-section">
                <h3>📦 Ruby Gems Example</h3>
                <div class="code-block">
# Sample Ruby code structure<br/>
class WebServerResponse<br/>
&nbsp;&nbsp;def initialize(cgi)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;@cgi = cgi<br/>
&nbsp;&nbsp;&nbsp;&nbsp;@timestamp = Time.now<br/>
&nbsp;&nbsp;end<br/>
<br/>
&nbsp;&nbsp;def to_json<br/>
&nbsp;&nbsp;&nbsp;&nbsp;{<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;ruby_version: RUBY_VERSION,<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;timestamp: @timestamp.iso8601,<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;method: ENV['REQUEST_METHOD']<br/>
&nbsp;&nbsp;&nbsp;&nbsp;}.to_json<br/>
&nbsp;&nbsp;end<br/>
end
                </div>
            </div>
            
            <p><em>✨ Crafted with Ruby elegance and CGI magic ✨</em></p>
        </div>
    </div>
</body>
</html>
HTML
