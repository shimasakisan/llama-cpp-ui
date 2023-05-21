import ConversationStep from "./ConversationStep";
//import EmptyConversation from "./EmptyConversation"

export default function Conversation({conversation}) {
    // if (!conversation || !conversation.steps || conversation.steps.length === 0) {
    //     return <EmptyConversation />
    // }
    
    return conversation.steps.map((step, i) => <ConversationStep key={i} {...step} />)
}