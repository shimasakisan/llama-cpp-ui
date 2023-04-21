import './EmptyConversation.css';

export default function EmptyConversation() {
    return (
        <div className="EmptyConversation">
            <p className="First">This is a nice web UI for LLM models running on your local machine using <b>llama.cpp</b>. There are no conversations yet, so you are seeing this short intro.</p>
            <p className="Second">Start a conversation with your local LLM model...</p>
        </div>
    )
}